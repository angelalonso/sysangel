use anyhow::{Context, Result};
use clap::Parser;
use indicatif::{MultiProgress, ProgressBar, ProgressStyle};
use num_cpus;
use rayon::prelude::*;
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};
use std::ffi::OsStr;
use std::fs::{self, File};
use std::io::{BufWriter, Write};
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

    /// Start from stage (1-3)
    #[arg(short, long, default_value_t = 1)]
    start_stage: u32,

    /// Quick hash size in bytes
    #[arg(long, default_value_t = 1024 * 1024)]
    quick_hash_size: usize,

    /// Output file for duplicates
    #[arg(short, long, default_value = "duplicates.txt")]
    output: PathBuf,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
struct Checkpoint {
    stage: u32,
    processed_files: HashSet<PathBuf>,
    candidate_pairs: Vec<(PathBuf, PathBuf)>,
    verified_pairs: Vec<(PathBuf, PathBuf)>,
    current_size: u64,
    obvious_duplicates: Vec<(PathBuf, PathBuf)>,
}

impl Default for Checkpoint {
    fn default() -> Self {
        Self {
            stage: 1,
            processed_files: HashSet::new(),
            candidate_pairs: Vec::new(),
            verified_pairs: Vec::new(),
            current_size: 0,
            obvious_duplicates: Vec::new(),
        }
    }
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

        let checkpoint = Self::load_checkpoint(&args.work_dir)?;

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
            serde_json::from_str(&data).context("Failed to parse checkpoint")
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

    fn scan_files(&self) -> Result<HashMap<u64, Vec<PathBuf>>> {
        println!("Stage 1: Scanning files...");

        let size_map = Arc::new(Mutex::new(HashMap::new()));
        let processed_count = Arc::new(AtomicU64::new(0));

        let pb = self.multiprogress.add(ProgressBar::new_spinner());
        pb.set_style(
            ProgressStyle::default_spinner()
                .template("{spinner} {msg} {pos} files scanned...")
                .unwrap()
                .tick_chars("⠁⠂⠄⡀⢀⠠⠐⠈ "),
        );
        pb.set_message("Scanning");

        let folders = vec![&self.args.folder1, &self.args.folder2];
        let checkpoint = self.checkpoint.clone();

        folders.par_iter().for_each(|folder| {
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
                        
                        let mut map = size_map.lock().unwrap();
                        map.entry(size).or_insert_with(Vec::new).push(path.clone());
                        
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

        let size_map = Arc::try_unwrap(size_map)
            .map_err(|_| anyhow::anyhow!("Failed to unwrap size map"))?
            .into_inner()
            .map_err(|_| anyhow::anyhow!("Failed to get size map"))?;

        self.save_checkpoint()?;
        Ok(size_map)
    }

    fn find_duplicates_prioritized(&self, size_map: &HashMap<u64, Vec<PathBuf>>) -> Result<(Vec<(PathBuf, PathBuf)>, Vec<(PathBuf, PathBuf)>)> {
        println!("Stage 2: Finding duplicate candidates...");

        let mut obvious_duplicates = Vec::new();
        let mut candidate_pairs = Vec::new();

        // Get checkpoint state
        let checkpoint = self.checkpoint.lock().unwrap();
        let start_size = checkpoint.current_size;
        let mut processed_pairs = checkpoint.candidate_pairs.clone();
        drop(checkpoint);

        // Prioritize sizes: larger files first, then uncommon sizes
        let mut sizes: Vec<u64> = size_map.keys().copied().collect();
        sizes.par_sort_by(|a, b| {
            // Larger files first, then by number of files (uncommon sizes first)
            b.cmp(a)
                .then(size_map[a].len().cmp(&size_map[b].len()))
        });

        let pb = self.multiprogress.add(ProgressBar::new(sizes.len() as u64));
        pb.set_style(
            ProgressStyle::default_bar()
                .template("{wide_bar} {pos}/{len} sizes ({eta})")
                .unwrap()
                .progress_chars("█▉▊▋▌▍▎▏  "),
        );

        for (i, &size) in sizes.iter().enumerate() {
            if size < start_size {
                pb.inc(1);
                continue;
            }

            let files = &size_map[&size];
            
            // Split files by folder - take ownership to avoid reference issues
            let files1: Vec<PathBuf> = files.iter()
                .filter(|p| p.starts_with(&self.args.folder1))
                .cloned()
                .collect();
            let files2: Vec<PathBuf> = files.iter()
                .filter(|p| p.starts_with(&self.args.folder2))
                .cloned()
                .collect();

            if files1.is_empty() || files2.is_empty() {
                pb.inc(1);
                continue;
            }

            // Find obvious duplicates (same filename) - use owned values
            let name_map1: HashMap<_, _> = files1
                .iter()
                .map(|p| (p.file_name().unwrap_or(OsStr::new("")).to_owned(), p.clone()))
                .collect();
            let name_map2: HashMap<_, _> = files2
                .iter()
                .map(|p| (p.file_name().unwrap_or(OsStr::new("")).to_owned(), p.clone()))
                .collect();

            for (name, file1) in &name_map1 {
                if let Some(file2) = name_map2.get(name) {
                    let pair = (file1.clone(), file2.clone());
                    if !processed_pairs.contains(&pair) {
                        obvious_duplicates.push(pair.clone());
                        processed_pairs.push(pair);
                    }
                }
            }

            // Generate all other candidate pairs
            for file1 in &files1 {
                for file2 in &files2 {
                    if file1.file_name() != file2.file_name() {
                        let pair = (file1.clone(), file2.clone());
                        if !processed_pairs.contains(&pair) {
                            candidate_pairs.push(pair.clone());
                            processed_pairs.push(pair);
                        }
                    }
                }
            }

            // Update checkpoint
            {
                let mut checkpoint = self.checkpoint.lock().unwrap();
                checkpoint.current_size = size;
                checkpoint.candidate_pairs = processed_pairs.clone();
                checkpoint.obvious_duplicates = obvious_duplicates.clone();
            }

            if i % 100 == 0 {
                self.save_checkpoint()?;
            }

            pb.inc(1);
        }

        pb.finish_with_message("Candidate search complete");
        self.save_checkpoint()?;

        Ok((obvious_duplicates, candidate_pairs))
    }

    fn quick_verify(&self, pairs: &[(PathBuf, PathBuf)]) -> Result<Vec<(PathBuf, PathBuf)>> {
        println!("Stage 3: Quick hash verification...");

        let checkpoint = self.checkpoint.lock().unwrap();
        let mut verified_pairs = checkpoint.verified_pairs.clone();
        drop(checkpoint);

        // Filter out already verified pairs
        let unverified_pairs: Vec<_> = pairs
            .iter()
            .filter(|p| !verified_pairs.contains(p))
            .cloned()
            .collect();

        let pb = self.multiprogress.add(ProgressBar::new(unverified_pairs.len() as u64));
        pb.set_style(
            ProgressStyle::default_bar()
                .template("{wide_bar} {pos}/{len} pairs ({eta})")
                .unwrap()
                .progress_chars("█▉▊▋▌▍▎▏  "),
        );

        let verified = Arc::new(Mutex::new(Vec::new()));
        let quick_hash_size = self.args.quick_hash_size;

        unverified_pairs.par_iter().for_each(|(file1, file2)| {
            if let (Some(hash1), Some(hash2)) = (
                Self::quick_hash(file1, quick_hash_size),
                Self::quick_hash(file2, quick_hash_size),
            ) {
                if hash1 == hash2 {
                    let mut verified = verified.lock().unwrap();
                    verified.push((file1.clone(), file2.clone()));
                }
            }
            pb.inc(1);
        });

        pb.finish_with_message("Quick verification complete");

        let result = verified.lock().unwrap().clone();
        verified_pairs.extend(result.clone());

        // Update checkpoint
        {
            let mut checkpoint = self.checkpoint.lock().unwrap();
            checkpoint.verified_pairs = verified_pairs;
            checkpoint.stage = 3;
        }

        self.save_checkpoint()?;
        Ok(result)
    }

    fn quick_hash(path: &Path, size: usize) -> Option<Vec<u8>> {
        let data = std::fs::read(path).ok()?;
        Some(blake3::hash(&data[..size.min(data.len())]).as_bytes().to_vec())
    }

    fn full_verify(&self, pairs: &[(PathBuf, PathBuf)]) -> Result<Vec<(PathBuf, PathBuf)>> {
        println!("Stage 4: Full hash verification...");

        let pb = self.multiprogress.add(ProgressBar::new(pairs.len() as u64));
        pb.set_style(
            ProgressStyle::default_bar()
                .template("{wide_bar} {pos}/{len} pairs ({eta})")
                .unwrap()
                .progress_chars("█▉▊▋▌▍▎▏  "),
        );

        let output_file = File::create(&self.args.output).context("Failed to create output file")?;
        let output_writer = Arc::new(Mutex::new(BufWriter::new(output_file)));
        let confirmed = Arc::new(Mutex::new(Vec::new()));

        pairs.par_iter().for_each(|(file1, file2)| {
            if let (Some(hash1), Some(hash2)) = (
                Self::full_hash(file1),
                Self::full_hash(file2),
            ) {
                if hash1 == hash2 {
                    let pair = (file1.clone(), file2.clone());
                    
                    // Write to file immediately
                    if let Ok(mut writer) = output_writer.lock() {
                        let _ = writeln!(writer, "{}\t{}", file1.display(), file2.display());
                    }
                    
                    let mut confirmed = confirmed.lock().unwrap();
                    confirmed.push(pair);
                }
            }
            pb.inc(1);
        });

        pb.finish_with_message("Full verification complete");

        // Ensure all data is written
        if let Ok(mut writer) = output_writer.lock() {
            let _ = writer.flush();
        }

        let result = confirmed.lock().unwrap().clone();

        // Clean up checkpoint on success
        let checkpoint_file = self.args.work_dir.join("checkpoint.json");
        if checkpoint_file.exists() {
            let _ = fs::remove_file(checkpoint_file);
        }

        Ok(result)
    }

    fn full_hash(path: &Path) -> Option<Vec<u8>> {
        let data = std::fs::read(path).ok()?;
        Some(blake3::hash(&data).as_bytes().to_vec())
    }

    fn run(&self) -> Result<()> {
        println!("Duplicate Finder - Rust Edition");
        println!("Folders: {} vs {}", self.args.folder1.display(), self.args.folder2.display());
        println!("Threads: {}", self.args.threads);
        println!("Starting from stage: {}", self.args.start_stage);

        rayon::ThreadPoolBuilder::new()
            .num_threads(self.args.threads)
            .build_global()
            .context("Failed to create thread pool")?;

        match self.args.start_stage {
            1 => {
                // Stage 1: Scan files
                let size_map = self.scan_files()?;
                println!("Found {} unique file sizes", size_map.len());

                // Stage 2: Find candidates
                let (obvious, candidates) = self.find_duplicates_prioritized(&size_map)?;
                println!("Found {} obvious duplicates", obvious.len());
                println!("Found {} candidate pairs", candidates.len());

                // Write obvious duplicates immediately
                if !obvious.is_empty() {
                    let mut file = File::create(&self.args.output).context("Failed to create output file")?;
                    for (f1, f2) in &obvious {
                        writeln!(file, "{}\t{}", f1.display(), f2.display())?;
                    }
                    file.flush()?;
                    println!("Written {} obvious duplicates to {}", obvious.len(), self.args.output.display());
                }

                let all_candidates = obvious.into_iter().chain(candidates).collect::<Vec<_>>();

                // Stage 3: Quick verify
                let quick_verified = self.quick_verify(&all_candidates)?;
                println!("Quick verification: {} pairs passed", quick_verified.len());

                // Stage 4: Full verify
                let final_duplicates = self.full_verify(&quick_verified)?;
                println!("Final result: {} confirmed duplicates", final_duplicates.len());

                if !final_duplicates.is_empty() {
                    println!("Results saved to: {}", self.args.output.display());
                }
            }
            2 => {
                // Resume from stage 2
                let checkpoint = self.checkpoint.lock().unwrap();
                let all_candidates = checkpoint.candidate_pairs.clone();
                drop(checkpoint);

                println!("Resuming with {} candidate pairs", all_candidates.len());

                let quick_verified = self.quick_verify(&all_candidates)?;
                let final_duplicates = self.full_verify(&quick_verified)?;
                println!("Final result: {} confirmed duplicates", final_duplicates.len());
            }
            3 => {
                // Resume from stage 3
                let checkpoint = self.checkpoint.lock().unwrap();
                let verified_pairs = checkpoint.verified_pairs.clone();
                drop(checkpoint);

                println!("Resuming with {} verified pairs", verified_pairs.len());
                let final_duplicates = self.full_verify(&verified_pairs)?;
                println!("Final result: {} confirmed duplicates", final_duplicates.len());
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
