// src/main.rs
use cloud_is_lava::{Cli, run};
use env_logger::Env;
use clap::Parser;

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    // Initialize logging
    env_logger::Builder::from_env(Env::default().default_filter_or("info")).init();
    
    log::info!("Starting CloudIsLava - Secure Sync System");
    
    // Parse command line arguments
    let cli = Cli::parse();
    
    // Run the application
    if let Err(e) = run(cli).await {
        log::error!("Application error: {}", e);
        std::process::exit(1);
    }
    
    Ok(())
}
