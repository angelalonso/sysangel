use std::{
    collections::{HashMap, HashSet},
    fs,
    io::{self, Read},
    path::{Path, PathBuf},
    sync::mpsc::{self, Receiver, Sender},
    thread,
};

use eframe::egui;
use globset::{Glob, GlobSet, GlobSetBuilder};
use rfd::FileDialog;
use serde::{Deserialize, Serialize};
use similar::TextDiff;
use walkdir::WalkDir;

#[derive(Clone, Debug, Serialize, Deserialize)]
struct FileEntry {
    relative_path: String,
    size: u64,
    hash: String,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
enum DifferenceKind {
    OnlyInLeft,
    OnlyInRight,
    DifferentContent,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
struct Difference {
    relative_path: String,
    kind: DifferenceKind,
    left: Option<FileEntry>,
    right: Option<FileEntry>,
    selected: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
struct ContentMatch {
    source_path: String,
    matching_paths: Vec<String>,
    size: u64,
    hash: String,
    selected: bool,
}

enum WorkerMessage {
    Progress(String),
    CompareDone(Result<Vec<Difference>, String>),
    MatchesDone(Result<Vec<ContentMatch>, String>),
    CopyDone(Result<usize, String>),
}

#[derive(Default)]
struct ModFilesApp {
    compare_left: String,
    compare_right: String,
    match_source: String,
    match_target: String,
    copy_destination: String,
    ignore_patterns: String,
    differences: Vec<Difference>,
    matches: Vec<ContentMatch>,
    status: String,
    progress: Option<String>,
    tab: Tab,
    worker: Option<Receiver<WorkerMessage>>,
    pending_copy: Option<CopyRequest>,
    overwrite_dialog: bool,
    diff_text: String,
    selected_diff: Option<usize>,
}

#[derive(Default, PartialEq)]
enum Tab {
    #[default]
    Compare,
    ContentMatch,
}

struct CopyRequest {
    source: PathBuf,
    destination: PathBuf,
    paths: Vec<String>,
}

fn main() -> eframe::Result<()> {
    eframe::run_native(
        "GTR2 Mod Files",
        eframe::NativeOptions::default(),
        Box::new(|creation_context| {
            configure_ui(&creation_context.egui_ctx);
            Ok(Box::new(ModFilesApp::default()))
        }),
    )
}

fn configure_ui(ctx: &egui::Context) {
    ctx.set_pixels_per_point(1.2);
    let mut style = (*ctx.style()).clone();
    style.spacing.button_padding = egui::vec2(12.0, 8.0);
    style.spacing.interact_size = egui::vec2(40.0, 32.0);
    ctx.set_style(style);
}

impl eframe::App for ModFilesApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        self.poll_worker();
        if self.worker.is_some() {
            ctx.request_repaint();
        }

        egui::TopBottomPanel::top("tabs").show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.selectable_value(&mut self.tab, Tab::Compare, "Compare folders");
                ui.selectable_value(&mut self.tab, Tab::ContentMatch, "Find matching content");
            });
        });

        egui::CentralPanel::default().show(ctx, |ui| {
            match self.tab {
                Tab::Compare => self.compare_ui(ui),
                Tab::ContentMatch => self.match_ui(ui),
            }
            if let Some(progress) = &self.progress {
                ui.separator();
                ui.label(progress);
            } else if !self.status.is_empty() {
                ui.separator();
                ui.label(&self.status);
            }
        });

        if self.overwrite_dialog {
            self.overwrite_window(ctx);
        }
    }
}

impl ModFilesApp {
    fn compare_ui(&mut self, ui: &mut egui::Ui) {
        ui.heading("Compare two folders");
        folder_row(ui, "Left folder", &mut self.compare_left);
        folder_row(ui, "Right folder", &mut self.compare_right);
        ignore_row(ui, &mut self.ignore_patterns);

        ui.horizontal(|ui| {
            if ui.button("Compare").clicked() {
                self.start_compare();
            }
            if ui.button("Export results").clicked() {
                self.export_differences();
            }
        });

        if !self.differences.is_empty() {
            ui.separator();
            ui.label(format!("{} differences", self.differences.len()));
            selection_buttons(ui, &mut self.differences);
            folder_row(ui, "Copy selected files to", &mut self.copy_destination);
            if ui.button("Copy selected files").clicked() {
                self.prepare_difference_copy();
            }

            let mut clicked_diff = None;
            let mut requested_text_diff = None;
            egui::ScrollArea::vertical()
                .max_height(330.0)
                .show(ui, |ui| {
                    for index in 0..self.differences.len() {
                        let difference = &mut self.differences[index];
                        let selected = self.selected_diff == Some(index);
                        let is_changed =
                            matches!(difference.kind, DifferenceKind::DifferentContent);
                        let label = format!(
                            "{}: {}",
                            difference_label(&difference.kind),
                            difference.relative_path
                        );
                        ui.horizontal(|ui| {
                            ui.checkbox(&mut difference.selected, "");
                            if ui.selectable_label(selected, label).clicked() {
                                clicked_diff = Some(index);
                            }
                            if is_changed && ui.button("Show diff").clicked() {
                                requested_text_diff = Some(index);
                            }
                        });
                    }
                });
            if let Some(index) = clicked_diff {
                self.selected_diff = Some(index);
                self.diff_text.clear();
            }
            if let Some(index) = requested_text_diff {
                self.show_text_diff(index);
            }
            if !self.diff_text.is_empty() {
                ui.separator();
                ui.label("Text diff:");
                egui::ScrollArea::both().max_height(260.0).show(ui, |ui| {
                    ui.monospace(&self.diff_text);
                });
            }
        }
    }

    fn match_ui(&mut self, ui: &mut egui::Ui) {
        ui.heading("Find files with identical content");
        folder_row(ui, "Source folder", &mut self.match_source);
        folder_row(ui, "Folder to search", &mut self.match_target);
        ignore_row(ui, &mut self.ignore_patterns);

        ui.horizontal(|ui| {
            if ui.button("Find matches").clicked() {
                self.start_match_search();
            }
            if ui.button("Export results").clicked() {
                self.export_matches();
            }
        });

        if !self.matches.is_empty() {
            ui.separator();
            ui.label(format!("{} source files with matches", self.matches.len()));
            selection_buttons(ui, &mut self.matches);
            folder_row(ui, "Copy selected files to", &mut self.copy_destination);
            if ui.button("Copy selected files").clicked() {
                self.prepare_match_copy();
            }
            egui::ScrollArea::vertical()
                .max_height(420.0)
                .show(ui, |ui| {
                    for item in &mut self.matches {
                        ui.horizontal(|ui| {
                            ui.checkbox(&mut item.selected, "");
                            ui.label(format!(
                                "{} ({} match{})",
                                item.source_path,
                                item.matching_paths.len(),
                                if item.matching_paths.len() == 1 {
                                    ""
                                } else {
                                    "es"
                                }
                            ));
                        });
                    }
                });
        }
    }

    fn start_compare(&mut self) {
        let left = PathBuf::from(&self.compare_left);
        let right = PathBuf::from(&self.compare_right);
        let ignore = match build_ignore_set(&self.ignore_patterns) {
            Ok(value) => value,
            Err(error) => {
                self.status = format!("Invalid ignore pattern: {error}");
                return;
            }
        };
        let (tx, rx) = mpsc::channel();
        self.differences.clear();
        self.matches.clear();
        self.diff_text.clear();
        self.begin_worker(rx, "Comparing folders...");
        thread::spawn(move || {
            let result = compare_folders(&left, &right, &ignore, &tx).map_err(|e| e.to_string());
            let _ = tx.send(WorkerMessage::CompareDone(result));
        });
    }

    fn start_match_search(&mut self) {
        let source = PathBuf::from(&self.match_source);
        let target = PathBuf::from(&self.match_target);
        let ignore = match build_ignore_set(&self.ignore_patterns) {
            Ok(value) => value,
            Err(error) => {
                self.status = format!("Invalid ignore pattern: {error}");
                return;
            }
        };
        let (tx, rx) = mpsc::channel();
        self.differences.clear();
        self.matches.clear();
        self.diff_text.clear();
        self.begin_worker(rx, "Searching for matching content...");
        thread::spawn(move || {
            let result = match (
                scan_folder(&source, &ignore, &tx),
                scan_folder(&target, &ignore, &tx),
            ) {
                (Ok(source_files), Ok(target_files)) => {
                    Ok(match_entries(&source_files, &target_files))
                }
                (Err(error), _) | (_, Err(error)) => Err(error.to_string()),
            };
            let _ = tx.send(WorkerMessage::MatchesDone(result));
        });
    }

    fn begin_worker(&mut self, worker: Receiver<WorkerMessage>, message: &str) {
        self.worker = Some(worker);
        self.progress = Some(message.to_string());
        self.status.clear();
    }

    fn poll_worker(&mut self) {
        let Some(worker) = self.worker.take() else {
            return;
        };
        let mut still_running = true;
        while let Ok(message) = worker.try_recv() {
            match message {
                WorkerMessage::Progress(message) => self.progress = Some(message),
                WorkerMessage::CompareDone(result) => {
                    still_running = false;
                    self.progress = None;
                    match result {
                        Ok(items) => {
                            self.status =
                                format!("Comparison complete: {} differences", items.len());
                            self.differences = items;
                        }
                        Err(error) => self.status = format!("Comparison failed: {error}"),
                    }
                }
                WorkerMessage::MatchesDone(result) => {
                    still_running = false;
                    self.progress = None;
                    match result {
                        Ok(items) => {
                            self.status = format!("Found {} matching source files", items.len());
                            self.matches = items;
                        }
                        Err(error) => self.status = format!("Search failed: {error}"),
                    }
                }
                WorkerMessage::CopyDone(result) => {
                    still_running = false;
                    self.progress = None;
                    self.status = match result {
                        Ok(count) => format!("Copied {count} file(s)"),
                        Err(error) => format!("Copy failed: {error}"),
                    };
                }
            }
        }
        if still_running {
            self.worker = Some(worker);
        }
    }

    fn prepare_difference_copy(&mut self) {
        let source = PathBuf::from(&self.compare_left);
        let destination = PathBuf::from(&self.copy_destination);
        let paths = self
            .differences
            .iter()
            .filter(|item| item.selected && !matches!(item.kind, DifferenceKind::OnlyInRight))
            .map(|item| item.relative_path.clone())
            .collect();
        self.prepare_copy(CopyRequest {
            source,
            destination,
            paths,
        });
    }

    fn prepare_match_copy(&mut self) {
        let source = PathBuf::from(&self.match_source);
        let destination = PathBuf::from(&self.copy_destination);
        let paths = self
            .matches
            .iter()
            .filter(|item| item.selected)
            .map(|item| item.source_path.clone())
            .collect();
        self.prepare_copy(CopyRequest {
            source,
            destination,
            paths,
        });
    }

    fn prepare_copy(&mut self, request: CopyRequest) {
        if request.paths.is_empty() {
            self.status = "No files selected".to_string();
            return;
        }
        let conflicts = request
            .paths
            .iter()
            .filter(|path| request.destination.join(path).is_file())
            .count();
        if conflicts > 0 {
            self.status = format!("{conflicts} destination file(s) already exist");
            self.pending_copy = Some(request);
            self.overwrite_dialog = true;
        } else {
            self.start_copy(request);
        }
    }

    fn overwrite_window(&mut self, ctx: &egui::Context) {
        egui::Window::new("Existing files")
            .collapsible(false)
            .resizable(false)
            .show(ctx, |ui| {
                ui.label("Some destination files already exist. What should happen?");
                ui.horizontal(|ui| {
                    if ui.button("Overwrite existing").clicked() {
                        self.confirm_copy(true);
                    }
                    if ui.button("Skip existing").clicked() {
                        self.confirm_copy(false);
                    }
                    if ui.button("Cancel").clicked() {
                        self.pending_copy = None;
                        self.overwrite_dialog = false;
                    }
                });
            });
    }

    fn confirm_copy(&mut self, overwrite: bool) {
        if let Some(mut request) = self.pending_copy.take() {
            if !overwrite {
                request
                    .paths
                    .retain(|path| !request.destination.join(path).is_file());
            }
            self.overwrite_dialog = false;
            if request.paths.is_empty() {
                self.status = "All selected files were skipped".to_string();
            } else {
                self.start_copy(request);
            }
        }
    }

    fn start_copy(&mut self, request: CopyRequest) {
        let (tx, rx) = mpsc::channel();
        self.begin_worker(rx, "Copying files...");
        thread::spawn(move || {
            let result = copy_paths(&request.source, &request.destination, &request.paths, &tx)
                .map_err(|e| e.to_string());
            let _ = tx.send(WorkerMessage::CopyDone(result));
        });
    }

    fn show_text_diff(&mut self, index: usize) {
        let difference = &self.differences[index];
        let (Some(left), Some(right)) = (&difference.left, &difference.right) else {
            return;
        };
        let left_path = Path::new(&self.compare_left).join(&left.relative_path);
        let right_path = Path::new(&self.compare_right).join(&right.relative_path);
        match (
            fs::read_to_string(&left_path),
            fs::read_to_string(&right_path),
        ) {
            (Ok(left_text), Ok(right_text))
                if left_text.len() <= 2_000_000 && right_text.len() <= 2_000_000 =>
            {
                self.diff_text = TextDiff::from_lines(&left_text, &right_text)
                    .unified_diff()
                    .header(
                        &left_path.display().to_string(),
                        &right_path.display().to_string(),
                    )
                    .to_string();
            }
            (Ok(_), Ok(_)) => {
                self.diff_text = "Text diff skipped: file is larger than 2 MB.".to_string()
            }
            _ => {
                self.diff_text =
                    "Text diff unavailable: one or both files are binary or unreadable.".to_string()
            }
        }
    }

    fn export_differences(&mut self) {
        if self.differences.is_empty() {
            self.status = "There are no comparison results to export".to_string();
            return;
        }
        if let Some(path) = FileDialog::new()
            .set_file_name("folder-differences.json")
            .save_file()
        {
            match serde_json::to_string_pretty(&self.differences)
                .and_then(|json| fs::write(&path, json).map_err(serde_json::Error::io))
            {
                Ok(()) => self.status = format!("Exported results to {}", path.display()),
                Err(error) => self.status = format!("Export failed: {error}"),
            }
        }
    }

    fn export_matches(&mut self) {
        if self.matches.is_empty() {
            self.status = "There are no content-match results to export".to_string();
            return;
        }
        if let Some(path) = FileDialog::new()
            .set_file_name("content-matches.json")
            .save_file()
        {
            match serde_json::to_string_pretty(&self.matches)
                .and_then(|json| fs::write(&path, json).map_err(serde_json::Error::io))
            {
                Ok(()) => self.status = format!("Exported results to {}", path.display()),
                Err(error) => self.status = format!("Export failed: {error}"),
            }
        }
    }
}

fn folder_row(ui: &mut egui::Ui, label: &str, value: &mut String) {
    ui.horizontal(|ui| {
        ui.label(label);
        ui.text_edit_singleline(value);
        if ui.button("Browse").clicked() {
            if let Some(path) = FileDialog::new().pick_folder() {
                *value = path.display().to_string();
            }
        }
    });
}

fn ignore_row(ui: &mut egui::Ui, value: &mut String) {
    ui.horizontal(|ui| {
        ui.label("Ignore patterns");
        ui.text_edit_singleline(value);
    });
    ui.small("Optional glob patterns separated by commas or new lines, e.g. *.bak, temp/**");
}

fn selection_buttons<T: Selectable>(ui: &mut egui::Ui, items: &mut [T]) {
    ui.horizontal(|ui| {
        if ui.button("Select all").clicked() {
            for item in items.iter_mut() {
                item.set_selected(true);
            }
        }
        if ui.button("Select none").clicked() {
            for item in items.iter_mut() {
                item.set_selected(false);
            }
        }
    });
}

trait Selectable {
    fn set_selected(&mut self, selected: bool);
}

impl Selectable for Difference {
    fn set_selected(&mut self, selected: bool) {
        self.selected = selected;
    }
}

impl Selectable for ContentMatch {
    fn set_selected(&mut self, selected: bool) {
        self.selected = selected;
    }
}

fn build_ignore_set(patterns: &str) -> Result<GlobSet, String> {
    let mut builder = GlobSetBuilder::new();
    for pattern in patterns
        .split(|character: char| character == ',' || character == '\n')
        .map(str::trim)
        .filter(|pattern| !pattern.is_empty())
    {
        builder.add(Glob::new(pattern).map_err(|error| error.to_string())?);
    }
    builder.build().map_err(|error| error.to_string())
}

fn scan_folder(
    root: &Path,
    ignore: &GlobSet,
    tx: &Sender<WorkerMessage>,
) -> io::Result<HashMap<String, FileEntry>> {
    if !root.is_dir() {
        return Err(io::Error::new(
            io::ErrorKind::NotFound,
            format!("not a folder: {}", root.display()),
        ));
    }
    let mut result = HashMap::new();
    for item in WalkDir::new(root).follow_links(false) {
        let item = item.map_err(io::Error::other)?;
        if !item.file_type().is_file() {
            continue;
        }
        let relative = item
            .path()
            .strip_prefix(root)
            .map_err(io::Error::other)?
            .to_string_lossy()
            .replace('\\', "/");
        if ignore.is_match(&relative) {
            continue;
        }
        let metadata = item.metadata()?;
        let _ = tx.send(WorkerMessage::Progress(format!("Hashing {relative}")));
        result.insert(
            relative.clone(),
            FileEntry {
                relative_path: relative,
                size: metadata.len(),
                hash: hash_file(item.path())?,
            },
        );
    }
    Ok(result)
}

fn compare_folders(
    left_path: &Path,
    right_path: &Path,
    ignore: &GlobSet,
    tx: &Sender<WorkerMessage>,
) -> io::Result<Vec<Difference>> {
    let left = scan_folder(left_path, ignore, tx)?;
    let right = scan_folder(right_path, ignore, tx)?;
    Ok(compare_entries(&left, &right))
}

fn hash_file(path: &Path) -> io::Result<String> {
    let mut file = fs::File::open(path)?;
    let mut hasher = blake3::Hasher::new();
    let mut buffer = [0; 1024 * 1024];
    loop {
        let count = file.read(&mut buffer)?;
        if count == 0 {
            break;
        }
        hasher.update(&buffer[..count]);
    }
    Ok(hasher.finalize().to_hex().to_string())
}

fn copy_paths(
    source: &Path,
    destination: &Path,
    paths: &[String],
    tx: &Sender<WorkerMessage>,
) -> io::Result<usize> {
    let mut copied = 0;
    for relative in paths {
        let from = source.join(relative);
        let to = destination.join(relative);
        if let Some(parent) = to.parent() {
            fs::create_dir_all(parent)?;
        }
        fs::copy(&from, &to)?;
        copied += 1;
        let _ = tx.send(WorkerMessage::Progress(format!("Copied {relative}")));
    }
    Ok(copied)
}

fn compare_entries(
    left: &HashMap<String, FileEntry>,
    right: &HashMap<String, FileEntry>,
) -> Vec<Difference> {
    let paths: HashSet<&String> = left.keys().chain(right.keys()).collect();
    let mut result = paths
        .into_iter()
        .filter_map(|path| match (left.get(path), right.get(path)) {
            (Some(left), Some(right)) if left.hash != right.hash => Some(Difference {
                relative_path: path.clone(),
                kind: DifferenceKind::DifferentContent,
                left: Some(left.clone()),
                right: Some(right.clone()),
                selected: true,
            }),
            (Some(left), None) => Some(Difference {
                relative_path: path.clone(),
                kind: DifferenceKind::OnlyInLeft,
                left: Some(left.clone()),
                right: None,
                selected: true,
            }),
            (None, Some(right)) => Some(Difference {
                relative_path: path.clone(),
                kind: DifferenceKind::OnlyInRight,
                left: None,
                right: Some(right.clone()),
                selected: false,
            }),
            _ => None,
        })
        .collect::<Vec<_>>();
    result.sort_by(|a, b| a.relative_path.cmp(&b.relative_path));
    result
}

fn match_entries(
    source: &HashMap<String, FileEntry>,
    target: &HashMap<String, FileEntry>,
) -> Vec<ContentMatch> {
    let mut by_hash: HashMap<(&str, u64), Vec<&FileEntry>> = HashMap::new();
    for entry in target.values() {
        by_hash
            .entry((entry.hash.as_str(), entry.size))
            .or_default()
            .push(entry);
    }
    let mut result = source
        .values()
        .filter_map(|entry| {
            let matching_paths = by_hash
                .get(&(entry.hash.as_str(), entry.size))?
                .iter()
                .map(|item| item.relative_path.clone())
                .collect::<Vec<_>>();
            Some(ContentMatch {
                source_path: entry.relative_path.clone(),
                matching_paths,
                size: entry.size,
                hash: entry.hash.clone(),
                selected: true,
            })
        })
        .collect::<Vec<_>>();
    result.sort_by(|a, b| a.source_path.cmp(&b.source_path));
    result
}

fn difference_label(kind: &DifferenceKind) -> &'static str {
    match kind {
        DifferenceKind::OnlyInLeft => "Only left",
        DifferenceKind::OnlyInRight => "Only right",
        DifferenceKind::DifferentContent => "Changed",
    }
}
