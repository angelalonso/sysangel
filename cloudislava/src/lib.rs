// src/lib.rs - Updated version
pub mod crypto;
pub mod dropbox;
pub mod models;
pub mod sync;
pub mod local;

use clap::Parser;
use std::path::{Path, PathBuf};
use anyhow::Context;
use dirs::home_dir;

#[derive(Parser)]
#[command(name = "CloudIsLava")]
#[command(about = "Secure, synchronized file storage with integrity verification")]
pub struct Cli {
    #[command(subcommand)]
    pub command: Commands,
    
    #[arg(long, default_value = "~/.cloudislava")]
    pub config_dir: String,
}

impl Cli {
    pub fn resolved_config_dir(&self) -> PathBuf {
        let path_str = &self.config_dir;
        if path_str.starts_with('~') {
            if let Some(home) = home_dir() {
                let rest = &path_str[1..];
                home.join(rest.trim_start_matches('/'))
            } else {
                PathBuf::from(path_str)
            }
        } else {
            PathBuf::from(path_str)
        }
    }
}

#[derive(clap::Subcommand)]
pub enum Commands {
    /// Initialize the system with a new vault
    Init {
        /// Dropbox access token
        #[arg(long)]
        token: Option<String>,
        
        /// Path to store local encrypted files
        #[arg(long, default_value = "./vault")]
        vault_path: PathBuf,
    },
    
    /// Add a file to the vault
    Add {
        /// Local file path to add
        path: PathBuf,
        
        /// Optional custom name in vault
        #[arg(long)]
        name: Option<String>,
    },
    
    /// Sync changes with Dropbox
    Sync {
        /// Force sync even if conflicts detected
        #[arg(long)]
        force: bool,
    },
    
    /// Get file from vault
    Get {
        /// File name in vault
        name: String,
        
        /// Optional output path
        #[arg(long)]
        output: Option<PathBuf>,
    },
    
    /// List files in vault
    List,
    
    /// Show sync status
    Status,
}

pub async fn run(cli: Cli) -> anyhow::Result<()> {
    let config_dir = cli.resolved_config_dir();
    
    // Ensure config directory exists
    std::fs::create_dir_all(&config_dir)
        .context("Failed to create config directory")?;
    
    match cli.command {
        Commands::Init { token, vault_path } => {
            init(&config_dir, &vault_path, token).await
        }
        Commands::Add { path, name } => {
            add_file(&config_dir, &path, name.as_deref()).await
        }
        Commands::Sync { force } => {
            sync(&config_dir, force).await
        }
        Commands::Get { name, output } => {
            get_file(&config_dir, &name, output.as_deref()).await
        }
        Commands::List => {
            list_files(&config_dir).await
        }
        Commands::Status => {
            show_status(&config_dir).await
        }
    }
}

// ... rest of the functions remain the same, just update them to use Path instead of &PathBuf
// Change function signatures from &PathBuf to &Path where appropriate

async fn init(config_dir: &Path, vault_path: &Path, token: Option<String>) -> anyhow::Result<()> {
    log::info!("Initializing CloudIsLava vault at: {:?}", vault_path);
    
    // Create vault directory
    std::fs::create_dir_all(vault_path)
        .context("Failed to create vault directory")?;
    
    // Create config file
    let config_path = config_dir.join("config.json");
    let config = Config {
        vault_path: vault_path.to_path_buf(),
        dropbox_token: token,
        device_id: uuid::Uuid::new_v4().to_string(),
        initialized_at: chrono::Utc::now(),
    };
    
    let config_json = serde_json::to_string_pretty(&config)?;
    std::fs::write(&config_path, config_json)
        .context("Failed to write config file")?;
    
    // Initialize local database
    let db_path = config_dir.join("cloudislava.db");
    let _db = local::db::LocalDatabase::new(&db_path)?;
    
    log::info!("Initialization complete!");
    log::info!("Config saved to: {:?}", config_path);
    log::info!("Vault location: {:?}", vault_path);
    
    Ok(())
}

async fn add_file(config_dir: &Path, file_path: &Path, name: Option<&str>) -> anyhow::Result<()> {
    log::info!("Adding file: {:?}", file_path);
    
    // Load config
    let config = load_config(config_dir)?;
    
    // Read file
    let file_data = std::fs::read(file_path)
        .context(format!("Failed to read file: {:?}", file_path))?;
    
    // Generate file ID
    let file_id = uuid::Uuid::new_v4().to_string();
    let file_name = name
        .map(|n| n.to_string())
        .unwrap_or_else(|| file_path.file_name()
            .unwrap_or_default()
            .to_string_lossy()
            .to_string());
    
    // Create crypto manager (in real app, derive from user password)
    let master_key = derive_master_key()?;
    let crypto = crypto::CryptoManager::new(&master_key);
    
    // Encrypt file
    let encrypted = crypto::encryption::FileEncryptor::encrypt(&file_data, &crypto, &file_id)?;
    
    // Create metadata
    let metadata = models::FileMetadata {
        id: file_id.clone(),
        name: file_name,
        size: file_data.len() as u64,
        version: 1,
        content_hash: crypto::integrity::IntegrityVerifier::content_hash(&file_data),
        created_at: chrono::Utc::now(),
        modified_at: chrono::Utc::now(),
        last_sync_at: None,
        signatures: Vec::new(),
        parent_id: None,
    };
    
    // Save encrypted file locally
    let encrypted_path = config.vault_path.join(&file_id);
    let encrypted_data = serde_json::to_vec(&encrypted)?;
    std::fs::write(&encrypted_path, encrypted_data)
        .context("Failed to save encrypted file")?;
    
    // Save metadata
    let metadata_path = config.vault_path.join(format!("{}.meta.json", file_id));
    let metadata_json = serde_json::to_string_pretty(&metadata)?;
    std::fs::write(&metadata_path, metadata_json)
        .context("Failed to save metadata")?;
    
    log::info!("File added successfully. ID: {}", file_id);
    
    Ok(())
}

async fn sync(config_dir: &Path, _force: bool) -> anyhow::Result<()> {
    log::info!("Starting sync");
    
    let config = load_config(config_dir)?;
    
    if config.dropbox_token.is_none() {
        return Err(anyhow::anyhow!("Dropbox token not configured. Run 'init' with --token"));
    }
    
    log::info!("Sync functionality will be implemented in next version");
    // TODO: Implement actual sync
    
    Ok(())
}

async fn get_file(config_dir: &Path, file_name: &str, _output: Option<&Path>) -> anyhow::Result<()> {
    log::info!("Retrieving file: {}", file_name);
    
    let _config = load_config(config_dir)?;
    
    // For now, just demonstrate finding and decrypting a file
    // In full implementation, you'd search by name or ID
    
    log::info!("Get functionality will be implemented in next version");
    
    Ok(())
}

async fn list_files(config_dir: &Path) -> anyhow::Result<()> {
    let config = load_config(config_dir)?;
    
    log::info!("Files in vault at: {:?}", config.vault_path);
    
    // List all files in vault
    let entries = std::fs::read_dir(&config.vault_path)?;
    
    let mut file_count = 0;
    for entry in entries {
        let entry = entry?;
        let path = entry.path();
        
        if path.is_file() {
            if let Some(ext) = path.extension() {
                if ext == "meta.json" {
                    // This is a metadata file
                    if let Ok(metadata_json) = std::fs::read_to_string(&path) {
                        if let Ok(metadata) = serde_json::from_str::<models::FileMetadata>(&metadata_json) {
                            println!("- {} (ID: {}, Size: {} bytes, Version: {})", 
                                metadata.name, metadata.id, metadata.size, metadata.version);
                            file_count += 1;
                        }
                    }
                }
            }
        }
    }
    
    if file_count == 0 {
        println!("No files found in vault");
    } else {
        println!("Total files: {}", file_count);
    }
    
    Ok(())
}

async fn show_status(config_dir: &Path) -> anyhow::Result<()> {
    let config = load_config(config_dir)?;
    
    println!("CloudIsLava Status");
    println!("==================");
    println!("Config directory: {:?}", config_dir);
    println!("Vault path: {:?}", config.vault_path);
    println!("Dropbox token configured: {}", config.dropbox_token.is_some());
    println!("Device ID: {}", config.device_id);
    println!("Initialized at: {}", config.initialized_at);
    
    // Check vault directory
    if config.vault_path.exists() {
        let entries = std::fs::read_dir(&config.vault_path)?;
        let file_count = entries.count();
        println!("Files in vault: {}", file_count);
    } else {
        println!("Vault directory does not exist!");
    }
    
    Ok(())
}

// Helper functions
fn load_config(config_dir: &Path) -> anyhow::Result<Config> {
    let config_path = config_dir.join("config.json");
    let config_data = std::fs::read_to_string(&config_path)
        .context("Config file not found. Run 'init' first")?;
    
    serde_json::from_str(&config_data)
        .context("Failed to parse config file")
}

fn derive_master_key() -> anyhow::Result<[u8; 32]> {
    // For demo purposes - in real app, derive from user password
    let mut key = [0u8; 32];
    getrandom::getrandom(&mut key)
        .map_err(|e| anyhow::anyhow!("Failed to generate random key: {}", e))?;
    Ok(key)
}

#[derive(Debug, serde::Serialize, serde::Deserialize)]
struct Config {
    vault_path: PathBuf,
    dropbox_token: Option<String>,
    device_id: String,
    initialized_at: chrono::DateTime<chrono::Utc>,
}
