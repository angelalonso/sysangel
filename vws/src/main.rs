use ignore::WalkBuilder;
use rayon::prelude::*;
use regex::Regex;
use std::collections::{HashMap, HashSet};
use std::env;
use std::fs;
use std::fs::File;
use std::io::{self, BufRead, BufReader, Write};
use std::path::PathBuf;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::Instant;

struct SearchResult {
    file: PathBuf,
    line_number: usize,
    context_lines: Vec<String>,
    match_line_index: usize,
}

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} <pattern> [directory] [options]", args[0]);
        eprintln!("  pattern     Search pattern");
        eprintln!("  directory   Optional, defaults to current directory (.)");
        eprintln!("\nOptions:");
        eprintln!("  -A NUM      Show NUM lines after match (default: 2)");
        eprintln!("  -B NUM      Show NUM lines before match (default: 2)");
        eprintln!("  -C NUM      Show NUM lines before and after match (default: 2)");
        eprintln!("  -v          Verbose mode (show context lines with formatting)");
        eprintln!("  -a          Enable thesaurus mode (search with related words)");
        std::process::exit(1);
    }

    let mut arg_idx = 1;
    
    // First argument is always the pattern
    let pattern = &args[arg_idx];
    arg_idx += 1;
    
    // Default values
    let mut directory = ".";
    let mut before = 4;
    let mut after = 7;
    let mut related_mode = false;
    let mut verbose_mode = false;
    
    // Parse remaining arguments
    while arg_idx < args.len() {
        match args[arg_idx].as_str() {
            "-A" => {
                if arg_idx + 1 < args.len() {
                    after = args[arg_idx + 1].parse().unwrap_or_else(|_| {
                        eprintln!("Error: -A requires a number");
                        std::process::exit(1);
                    });
                    arg_idx += 2;
                } else {
                    eprintln!("Error: -A requires a number");
                    std::process::exit(1);
                }
            }
            "-B" => {
                if arg_idx + 1 < args.len() {
                    before = args[arg_idx + 1].parse().unwrap_or_else(|_| {
                        eprintln!("Error: -B requires a number");
                        std::process::exit(1);
                    });
                    arg_idx += 2;
                } else {
                    eprintln!("Error: -B requires a number");
                    std::process::exit(1);
                }
            }
            "-C" => {
                if arg_idx + 1 < args.len() {
                    let num: usize = args[arg_idx + 1].parse().unwrap_or_else(|_| {
                        eprintln!("Error: -C requires a number");
                        std::process::exit(1);
                    });
                    before = num;
                    after = num;
                    arg_idx += 2;
                } else {
                    eprintln!("Error: -C requires a number");
                    std::process::exit(1);
                }
            }
            "-v" => {
                verbose_mode = true;
                arg_idx += 1;
            }
            "-a" => {
                related_mode = true;
                arg_idx += 1;
            }
            _ => {
                // If it doesn't start with '-', treat as directory
                if !args[arg_idx].starts_with('-') {
                    directory = &args[arg_idx];
                    arg_idx += 1;
                } else {
                    eprintln!("Warning: Unknown flag '{}'", args[arg_idx]);
                    arg_idx += 1;
                }
            }
        }
    }

    if verbose_mode {
        println!("🔍 Searching for: '{}'", pattern);
        if related_mode {
            println!("📖 Related words mode enabled");
        }
        println!("📁 In directory: {}", directory);
        println!("📄 Context: {} lines before, {} lines after", before, after);
        println!();
    }

    let start = Instant::now();
    
    let regex_pattern = if related_mode {
        let thesaurus = load_thesaurus();
        let related = get_related_words(pattern, &thesaurus);
        
        if verbose_mode && related.len() > 1 {
            println!("📚 Related words: {}", related.join(", "));
        }
        
        format!(r"(?i)\b({})\b", related.join("|"))
    } else {
        format!(r"(?i){}", regex::escape(pattern))
    };
    
    let regex = Regex::new(&regex_pattern).unwrap_or_else(|e| {
        eprintln!("Invalid regex pattern: {}", e);
        std::process::exit(1);
    });

    let match_count = AtomicUsize::new(0);
    let file_count = AtomicUsize::new(0);

    let files: Vec<_> = WalkBuilder::new(directory)
        .hidden(false)
        .git_ignore(true)
        .ignore(true)
        .build()
        .filter_map(|e| e.ok())
        .filter(|e| e.file_type().map(|ft| ft.is_file()).unwrap_or(false))
        .map(|e| e.into_path())
        .collect();

    if verbose_mode {
        println!("📊 Found {} files to search", files.len());
        println!();
    }

    let results: Vec<_> = files
        .par_iter()
        .filter_map(|file| {
            if verbose_mode {
                file_count.fetch_add(1, Ordering::Relaxed);
                
                if file_count.load(Ordering::Relaxed) % 1000 == 0 {
                    print!("\r⏳ Processed {} files...", file_count.load(Ordering::Relaxed));
                    let _ = io::stdout().flush();
                }
            }

            search_file(file, &regex, before, after).map(|mut results| {
                match_count.fetch_add(results.len(), Ordering::Relaxed);
                for r in &mut results {
                    r.file = file.clone();
                }
                results
            })
        })
        .flatten()
        .collect();

    if verbose_mode {
        println!("\r✅ Processed {} files", file_count.load(Ordering::Relaxed));
        println!();
    }

    if results.is_empty() {
        if verbose_mode {
            println!("❌ No matches found");
        }
    } else {
        if verbose_mode {
            println!("🎯 Found {} matches:\n", results.len());
        }
        
        let mut current_file = PathBuf::new();
        
        for result in results {
            if verbose_mode {
                if result.file != current_file {
                    current_file = result.file.clone();
                    println!("\n\x1b[1;33m{}:{}\x1b[0m", current_file.display(), result.line_number);
                } else {
                    println!("  \x1b[2;37m{}\x1b[0m:\x1b[1;33m{}\x1b[0m", current_file.display(), result.line_number);
                }
                
                for (idx, line) in result.context_lines.iter().enumerate() {
                    if idx == result.match_line_index {
                        print!("\x1b[1;37m");
                        
                        let highlighted = regex.replace_all(line, |caps: &regex::Captures| {
                            format!("\x1b[1;34m{}\x1b[1;37m", &caps[0])
                        });
                        
                        println!("{}", highlighted);
                        print!("\x1b[0m");
                    } else {
                        println!("\x1b[2;37m{}\x1b[0m", line);
                    }
                }
                println!();
            } else {
                println!("{}:{}", result.file.display(), result.line_number);
            }
        }
    }

    if verbose_mode {
        println!("\n📈 Summary:");
        println!("   Time: {:?}", start.elapsed());
        println!("   Files: {}", file_count.load(Ordering::Relaxed));
        println!("   Matches: {}", match_count.load(Ordering::Relaxed));
        println!("   Context: {} before, {} after", before, after);
        
        if related_mode {
            let thesaurus = load_thesaurus();
            let related = get_related_words(pattern, &thesaurus);
            println!("   Related words: {}", related.join(", "));
        }
    }

    Ok(())
}

fn load_thesaurus() -> HashMap<String, Vec<String>> {
    let mut thesaurus = HashMap::new();
    
    // First try current directory
    if let Ok(contents) = fs::read_to_string("./thesaurus.txt") {
        parse_thesaurus_content(&contents, &mut thesaurus);
    }
    
    // If not found, try ~/.vws/
    if thesaurus.is_empty() {
        let home = env::var("HOME").unwrap_or_default();
        let vws_path = format!("{}/.vws/thesaurus.txt", home);
        if let Ok(contents) = fs::read_to_string(vws_path) {
            parse_thesaurus_content(&contents, &mut thesaurus);
        }
    }
    
    // If still empty, use embedded default
    if thesaurus.is_empty() {
        // Built-in default thesaurus
        let default_content = r#"eye,vision,sight,see,view,look,eyeball,ocular
computer,pc,laptop,desktop,workstation,machine
file,document,text,note,archive,record
search,find,locate,lookup,query,scan
word,term,keyword,phrase,expression
note,memo,reminder,journal,diary,log
create,make,build,generate,produce
delete,remove,erase,eliminate
update,modify,change,edit
view,display,show,visualize
help,assist,support,guide
error,mistake,bug,failure,issue"#;
        
        parse_thesaurus_content(default_content, &mut thesaurus);
    }
    
    thesaurus
}

fn parse_thesaurus_content(content: &str, thesaurus: &mut HashMap<String, Vec<String>>) {
    for line in content.lines() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        
        let words: Vec<String> = line.split(',')
            .map(|s| s.trim().to_lowercase())
            .filter(|s| !s.is_empty())
            .collect();
        
        if !words.is_empty() {
            let key = words[0].clone();
            thesaurus.insert(key, words);
        }
    }
}

fn get_related_words(word: &str, thesaurus: &HashMap<String, Vec<String>>) -> Vec<String> {
    let word_lower = word.to_lowercase();
    let mut related = HashSet::new();
    
    related.insert(word_lower.clone());
    
    if let Some(synonyms) = thesaurus.get(&word_lower) {
        for w in synonyms {
            related.insert(w.clone());
        }
    } else {
        for (key, synonyms) in thesaurus {
            if key.contains(&word_lower) || word_lower.contains(key) {
                for w in synonyms {
                    related.insert(w.clone());
                }
            }
        }
    }
    
    let mut result: Vec<String> = related.into_iter().collect();
    result.sort();
    result
}

fn search_file(
    path: &std::path::Path, 
    regex: &Regex, 
    before: usize, 
    after: usize
) -> Option<Vec<SearchResult>> {
    let file = File::open(path).ok()?;
    let reader = BufReader::new(file);
    
    let lines: Vec<String> = reader
        .lines()
        .filter_map(|l| l.ok())
        .collect();

    let mut results = Vec::new();

    for (i, line) in lines.iter().enumerate() {
        if regex.is_match(line) {
            let start = i.saturating_sub(before);
            let end = (i + after + 1).min(lines.len());
            
            let context_lines: Vec<String> = lines[start..end]
                .iter()
                .map(|l| l.clone())
                .collect();

            results.push(SearchResult {
                file: path.to_path_buf(),
                line_number: i + 1,
                context_lines,
                match_line_index: i - start,
            });
        }
    }

    if results.is_empty() { None } else { Some(results) }
}
