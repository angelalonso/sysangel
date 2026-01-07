use anyhow::{Context, Result};
use clap::Parser;
use indicatif::{MultiProgress, ProgressBar, ProgressStyle};
use num_cpus;
use rayon::prelude::*;
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};
use std::ffi::OsStr;
use std::fs::{self, File};
use std::io::Write;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Mutex};
use walkdir::WalkDir;

#[derive(Parser, Debug)]
#[command(author, version, about, long_about = None)]
struct Args {
    /// First folder to compare
    folder1: PathBuf,

    /// Second folder to compare
    folder2: PathBuf,

    /// Work directory for temporary files
    #[arg(short, long, default_value = "./dup_finder_work")]
    work_dir: PathBuf,

    /// Number of threads to use
    #[arg(short, long, default_value_t = num_cpus::get())]
    threads: usize,

    /// Start from stage (1-2)
    #[arg(short, long, default_value_t = 1)]
    start_stage: u32,

    /// Quick hash size in bytes
    #[arg(long, default_value_t = 1024 * 1024)]
    quick_hash_size: usize,

    /// Maximum file size for phase 2 (bytes)
    #[arg(long, default_value_t = 100 * 1024 * 1024)]
    max_phase2_size: u64,

    /// Output file for duplicates
    #[arg(short, long, default_value = "duplicates.txt")]
    output: PathBuf,

    /// Force restart and ignore existing checkpoint
    #[arg(short, long)]
    force: bool,
}

#[derive(Serialize, Deserialize, Clone, Debug, Default)]
struct Checkpoint {
    #[serde(default)]
    stage: u32,
    #[serde(default)]
    processed_files: HashSet<PathBuf>,
    #[serde(default)]
    phase1_results: Vec<(PathBuf, PathBuf)>,
    #[serde(default)]
    phase2_candidates: Vec<PathBuf>,
    #[serde(default)]
    phase2_results: Vec<(PathBuf, PathBuf)>,
    #[serde(default)]
    current_size: u64,
}

#[derive(Clone)]
struct FileInfo {
    path: PathBuf,
    size: u64,
    name: String,
}

struct DuplicateFinder {
    args: Args,
    checkpoint: Arc<Mutex<Checkpoint>>,
    multiprogress: MultiProgress,
}

impl DuplicateFinder {
    fn new(args: Args) -> Result<Self> {
        // Create work directory
        fs::create_dir_all(&args.work_dir).context("Failed to create work directory")?;

        let checkpoint = if args.force {
            // Remove existing checkpoint if force flag is set
            let checkpoint_file = args.work_dir.join("checkpoint.json");
            if checkpoint_file.exists() {
                fs::remove_file(&checkpoint_file).context("Failed to remove existing checkpoint")?;
                println!("Removed existing checkpoint file");
            }
            Checkpoint::default()
        } else {
            Self::load_checkpoint(&args.work_dir)?
        };

        Ok(Self {
            args,
            checkpoint: Arc::new(Mutex::new(checkpoint)),
            multiprogress: MultiProgress::new(),
        })
    }

    fn load_checkpoint(work_dir: &Path) -> Result<Checkpoint> {
        let checkpoint_file = work_dir.join("checkpoint.json");
        if checkpoint_file.exists() {
            let data = fs::read_to_string(&checkpoint_file).context("Failed to read checkpoint")?;
            match serde_json::from_str(&data) {
                Ok(checkpoint) => Ok(checkpoint),
                Err(e) => {
                    eprintln!("Warning: Failed to parse checkpoint file: {}", e);
                    eprintln!("Starting with fresh checkpoint...");
                    // If checkpoint is corrupted or from old version, start fresh
                    Ok(Checkpoint::default())
                }
            }
        } else {
            Ok(Checkpoint::default())
        }
    }

    fn save_checkpoint(&self) -> Result<()> {
        let checkpoint_file = self.args.work_dir.join("checkpoint.json");
        let checkpoint = self.checkpoint.lock().unwrap();
        let data = serde_json::to_string_pretty(&*checkpoint).context("Failed to serialize checkpoint")?;
        fs::write(checkpoint_file, data).context("Failed to write checkpoint")
    }

    fn scan_files(&self) -> Result<(Vec<FileInfo>, Vec<FileInfo>)> {
        println!("Stage 1: Scanning files...");

        let folder1_files = Arc::new(Mutex::new(Vec::new()));
        let folder2_files = Arc::new(Mutex::new(Vec::new()));
        let processed_count = Arc::new(AtomicU64::new(0));

        let pb = self.multiprogress.add(ProgressBar::new_spinner());
        pb.set_style(
            ProgressStyle::default_spinner()
                .template("{spinner} {msg} {pos} files scanned...")
                .unwrap()
                .tick_chars("⠁⠂⠄⡀⢀⠠⠐⠈ "),
        );
        pb.set_message("Scanning");

        let folders = vec![(&self.args.folder1, folder1_files.clone()), (&self.args.folder2, folder2_files.clone())];
        let checkpoint = self.checkpoint.clone();

        folders.par_iter().for_each(|(folder, files_vec)| {
            let walker = WalkDir::new(folder).min_depth(1);
            
            for entry in walker.into_iter().filter_map(|e| e.ok()) {
                if entry.file_type().is_file() {
                    let path = entry.path().to_path_buf();
                    
                    // Check if already processed
                    {
                        let checkpoint = checkpoint.lock().unwrap();
                        if checkpoint.processed_files.contains(&path) {
                            processed_count.fetch_add(1, Ordering::Relaxed);
                            continue;
                        }
                    }

                    if let Ok(metadata) = entry.metadata() {
                        let size = metadata.len();
                        let name = path.file_name()
                            .unwrap_or(OsStr::new(""))
                            .to_string_lossy()
                            .to_string();
                        
                        let file_info = FileInfo { path: path.clone(), size, name };
                        
                        let mut files = files_vec.lock().unwrap();
                        files.push(file_info);
                        
                        // Add to processed files
                        let mut checkpoint = checkpoint.lock().unwrap();
                        checkpoint.processed_files.insert(path);
                    }
                }
                processed_count.fetch_add(1, Ordering::Relaxed);
                pb.set_position(processed_count.load(Ordering::Relaxed));
            }
        });

        pb.finish_with_message("Scanning complete");

        let folder1_files = Arc::try_unwrap(folder1_files)
            .map_err(|_| anyhow::anyhow!("Failed to unwrap folder1 files"))?
            .into_inner()
            .map_err(|_| anyhow::anyhow!("Failed to get folder1 files"))?;

        let folder2_files = Arc::try_unwrap(folder2_files)
            .map_err(|_| anyhow::anyhow!("Failed to unwrap folder2 files"))?
            .into_inner()
            .map_err(|_| anyhow::anyhow!("Failed to get folder2 files"))?;

        self.save_checkpoint()?;
        
        println!("Found {} files in folder1", folder1_files.len());
        println!("Found {} files in folder2", folder2_files.len());
        
        Ok((folder1_files, folder2_files))
    }

    fn phase1_find_obvious_duplicates(&self, folder1_files: &[FileInfo], folder2_files: &[FileInfo]) -> Result<Vec<(PathBuf, PathBuf)>> {
        println!("Phase 1: Finding obvious duplicates (same name + size)...");

        let pb = self.multiprogress.add(ProgressBar::new(folder1_files.len() as u64));
        pb.set_style(
            ProgressStyle::default_bar()
                .template("{wide_bar} {pos}/{len} files ({eta})")
                .unwrap()
                .progress_chars("█▉▊▋▌▍▎▏  "),
        );

        // Create lookup map for folder2 files by (name, size)
        let folder2_map: HashMap<_, _> = folder2_files
            .iter()
            .map(|f| ((f.name.clone(), f.size), f.path.clone()))
            .collect();

        let mut obvious_duplicates = Vec::new();

        for file1 in folder1_files {
            if let Some(file2_path) = folder2_map.get(&(file1.name.clone(), file1.size)) {
                obvious_duplicates.push((file1.path.clone(), file2_path.clone()));
            }
            pb.inc(1);
        }

        pb.finish_with_message("Phase 1 complete");

        // Update checkpoint
        {
            let mut checkpoint = self.checkpoint.lock().unwrap();
            checkpoint.phase1_results = obvious_duplicates.clone();
            checkpoint.stage = 2;
        }

        self.save_checkpoint()?;
        
        println!("Phase 1: Found {} obvious duplicates", obvious_duplicates.len());
        Ok(obvious_duplicates)
    }

    fn phase2_prepare_candidates(&self, folder1_files: &[FileInfo], folder2_files: &[FileInfo], obvious_duplicates: &[(PathBuf, PathBuf)]) -> Result<Vec<FileInfo>> {
        println!("Phase 2: Preparing candidates...");

        // Get files from folder2 that are NOT in obvious duplicates
        let obvious_paths: HashSet<_> = obvious_duplicates
            .iter()
            .map(|(_, f2)| f2)
            .collect();

        let candidates: Vec<FileInfo> = folder2_files
            .iter()
            .filter(|f| !obvious_paths.contains(&f.path) && f.size <= self.args.max_phase2_size)
            .cloned()
            .collect();

        // Sort by size (larger first for prioritization)
        let mut sorted_candidates = candidates;
        sorted_candidates.par_sort_by(|a, b| b.size.cmp(&a.size));

        // Update checkpoint
        {
            let mut checkpoint = self.checkpoint.lock().unwrap();
            checkpoint.phase2_candidates = sorted_candidates.iter().map(|f| f.path.clone()).collect();
        }

        self.save_checkpoint()?;
        
        println!("Phase 2: {} candidates prepared", sorted_candidates.len());
        Ok(sorted_candidates)
    }

    fn phase2_find_hidden_duplicates(&self, folder1_files: &[FileInfo], candidates: &[FileInfo]) -> Result<Vec<(PathBuf, PathBuf)>> {
        println!("Phase 2: Finding hidden duplicates...");

        // Create size index for folder1 files
        let folder1_size_index: HashMap<u64, Vec<&FileInfo>> = {
            let mut map: HashMap<u64, Vec<&FileInfo>> = HashMap::new();
            for file in folder1_files {
                map.entry(file.size).or_default().push(file);
            }
            map
        };

        let pb = self.multiprogress.add(ProgressBar::new(candidates.len() as u64));
        pb.set_style(
            ProgressStyle::default_bar()
                .template("{wide_bar} {pos}/{len} candidates ({eta})")
                .unwrap()
                .progress_chars("█▉▊▋▌▍▎▏  "),
        );

        let checkpoint = self.checkpoint.lock().unwrap();
        let processed_count = checkpoint.phase2_results.len();
        drop(checkpoint);

        let hidden_duplicates = Arc::new(Mutex::new(Vec::new()));

        candidates.par_iter().enumerate().for_each(|(i, candidate)| {
            if i < processed_count {
                pb.inc(1);
                return;
            }

            if let Some(folder1_matches) = folder1_size_index.get(&candidate.size) {
                for file1 in folder1_matches {
                    if Self::files_equal(&file1.path, &candidate.path, self.args.quick_hash_size) {
                        let mut duplicates = hidden_duplicates.lock().unwrap();
                        duplicates.push((file1.path.clone(), candidate.path.clone()));
                        break; // Found a match, no need to check other files with same size
                    }
                }
            }
            pb.inc(1);
        });

        pb.finish_with_message("Phase 2 complete");

        let result = hidden_duplicates.lock().unwrap().clone();

        // Update checkpoint
        {
            let mut checkpoint = self.checkpoint.lock().unwrap();
            checkpoint.phase2_results.extend(result.clone());
        }

        self.save_checkpoint()?;
        
        println!("Phase 2: Found {} hidden duplicates", result.len());
        Ok(result)
    }

    fn files_equal(file1: &Path, file2: &Path, quick_hash_size: usize) -> bool {
        if let (Some(hash1), Some(hash2)) = (
            Self::quick_hash(file1, quick_hash_size),
            Self::quick_hash(file2, quick_hash_size),
        ) {
            hash1 == hash2
        } else {
            false
        }
    }

    fn quick_hash(path: &Path, size: usize) -> Option<Vec<u8>> {
        let data = std::fs::read(path).ok()?;
        Some(blake3::hash(&data[..size.min(data.len())]).as_bytes().to_vec())
    }

    fn save_results(&self, phase1_results: &[(PathBuf, PathBuf)], phase2_results: &[(PathBuf, PathBuf)]) -> Result<()> {
        let mut file = File::create(&self.args.output).context("Failed to create output file")?;
        
        // Write header
        writeln!(file, "# Duplicate files found")?;
        writeln!(file, "# Phase 1 (obvious): {} files", phase1_results.len())?;
        writeln!(file, "# Phase 2 (hidden): {} files", phase2_results.len())?;
        writeln!(file, "# Total: {} duplicate pairs", phase1_results.len() + phase2_results.len())?;
        writeln!(file, "")?;

        // Write phase 1 results
        if !phase1_results.is_empty() {
            writeln!(file, "# Phase 1 - Obvious duplicates (same name + size)")?;
            for (f1, f2) in phase1_results {
                writeln!(file, "{}\t{}", f1.display(), f2.display())?;
            }
            writeln!(file, "")?;
        }

        // Write phase 2 results
        if !phase2_results.is_empty() {
            writeln!(file, "# Phase 2 - Hidden duplicates (same size + content)")?;
            for (f1, f2) in phase2_results {
                writeln!(file, "{}\t{}", f1.display(), f2.display())?;
            }
        }

        file.flush()?;
        
        println!("Results saved to: {}", self.args.output.display());
        Ok(())
    }

    fn run(&self) -> Result<()> {
        println!("Optimized Duplicate Finder - Rust Edition");
        println!("Folders: {} vs {}", self.args.folder1.display(), self.args.folder2.display());
        println!("Threads: {}", self.args.threads);
        println!("Max phase 2 size: {} MB", self.args.max_phase2_size / 1024 / 1024);
        println!("Starting from stage: {}", self.args.start_stage);

        rayon::ThreadPoolBuilder::new()
            .num_threads(self.args.threads)
            .build_global()
            .context("Failed to create thread pool")?;

        match self.args.start_stage {
            1 => {
                // Stage 1: Scan files
                let (folder1_files, folder2_files) = self.scan_files()?;

                // Phase 1: Find obvious duplicates
                let phase1_results = self.phase1_find_obvious_duplicates(&folder1_files, &folder2_files)?;

                // Phase 2: Find hidden duplicates
                let candidates = self.phase2_prepare_candidates(&folder1_files, &folder2_files, &phase1_results)?;
                let phase2_results = self.phase2_find_hidden_duplicates(&folder1_files, &candidates)?;

                // Save results
                self.save_results(&phase1_results, &phase2_results)?;

                // Clean up checkpoint on success
                let checkpoint_file = self.args.work_dir.join("checkpoint.json");
                if checkpoint_file.exists() {
                    let _ = fs::remove_file(checkpoint_file);
                }
            }
            2 => {
                // Resume from stage 2
                let checkpoint = self.checkpoint.lock().unwrap();
                let phase1_results = checkpoint.phase1_results.clone();
                drop(checkpoint);

                println!("Resuming with {} phase 1 results", phase1_results.len());

                // Need to rescan files to get FileInfo structures
                let (folder1_files, folder2_files) = self.scan_files()?;
                let candidates = self.phase2_prepare_candidates(&folder1_files, &folder2_files, &phase1_results)?;
                let phase2_results = self.phase2_find_hidden_duplicates(&folder1_files, &candidates)?;

                self.save_results(&phase1_results, &phase2_results)?;

                // Clean up checkpoint on success
                let checkpoint_file = self.args.work_dir.join("checkpoint.json");
                if checkpoint_file.exists() {
                    let _ = fs::remove_file(checkpoint_file);
                }
            }
            _ => anyhow::bail!("Invalid start stage: {}", self.args.start_stage),
        }

        Ok(())
    }
}

fn main() -> Result<()> {
    let args = Args::parse();

    // Validate folders
    if !args.folder1.exists() {
        anyhow::bail!("Folder1 does not exist: {}", args.folder1.display());
    }
    if !args.folder2.exists() {
        anyhow::bail!("Folder2 does not exist: {}", args.folder2.display());
    }

    let finder = DuplicateFinder::new(args)?;
    
    if let Err(e) = finder.run() {
        eprintln!("Error: {}", e);
        std::process::exit(1);
    }

    Ok(())
}
