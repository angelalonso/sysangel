use clap::{Parser, Subcommand};
use creds::commands::*;

#[derive(Parser)]
#[command(name = "creds")]
#[command(about = "A simple credential manager for developers")]
#[command(version, long_about = None)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
    
    /// Use a custom vault file
    #[arg(short, long, value_name = "FILE")]
    vault: Option<String>,
}

#[derive(Subcommand)]
enum Commands {
    /// Store a new secret
    Set {
        /// The key/name for the secret
        key: String,
        
        /// The value to store (prompted if not provided)
        value: Option<String>,
        
        /// Description for the secret
        #[arg(short, long)]
        description: Option<String>,
    },
    
    /// Retrieve a secret
    Get {
        /// The key/name of the secret
        key: String,
        
        /// Show metadata along with value
        #[arg(short, long)]
        metadata: bool,
    },
    
    /// List all stored secrets
    List {
        /// Show descriptions
        #[arg(short, long)]
        descriptions: bool,
    },
    
    /// Delete a secret
    Delete {
        /// The key/name to delete
        key: String,
        
        /// Skip confirmation prompt
        #[arg(short, long)]
        force: bool,
    },
    
    /// Export all secrets (for backup)
    Export {
        /// Output file (stdout if not specified)
        #[arg(short, long)]
        output: Option<String>,
    },
    
    /// Show vault location and info
    Info,
}

fn main() -> anyhow::Result<()> {
    let cli = Cli::parse();
    
    // Get master password
    println!("Enter master password: ");
    let password = rpassword::read_password()?;
    
    // Create vault
    let vault = if let Some(path) = cli.vault {
        creds::Vault::with_path(&password, Some(std::path::Path::new(&path)))?
    } else {
        creds::Vault::new(&password)?
    };
    
    match cli.command {
        Commands::Set { key, value, description } => {
            let cmd = SetCommand { key, value, description };
            cmd.execute(&vault)?;
        }
        Commands::Get { key, metadata } => {
            let cmd = GetCommand { key, show_metadata: metadata };
            cmd.execute(&vault)?;
        }
        Commands::List { descriptions } => {
            let cmd = ListCommand { show_descriptions: descriptions };
            cmd.execute(&vault)?;
        }
        Commands::Delete { key, force } => {
            let cmd = DeleteCommand { key, force };
            cmd.execute(&vault)?;
        }
        Commands::Export { output } => {
            let cmd = ExportCommand { output };
            cmd.execute(&vault)?;
        }
        Commands::Info => {
            println!("Vault location: {}", vault.location().display());
            let count = vault.list()?.len();
            println!("Secrets stored: {}", count);
        }
    }
    
    Ok(())
}
