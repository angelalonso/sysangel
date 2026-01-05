// examples/basic_usage.rs
use cloud_is_lava::CryptoManager;
use cloud_is_lava::sync::operations::SyncEngine;
use std::path::PathBuf;

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    // Initialize with a master key (in real app, derive from password)
    let master_key = b"32-byte-master-key-for-demo-only-!";
    let crypto = CryptoManager::new(master_key);
    
    // Initialize sync engine
    let db_path = PathBuf::from("./cloudislava.db");
    let local_db = LocalDatabase::new(&db_path)?;
    
    let dropbox_token = std::env::var("DROPBOX_TOKEN")?;
    let dropbox_client = CloudClient::new(dropbox_token, None);
    
    let mut sync_engine = SyncEngine {
        local_db,
        dropbox_client,
        offline_mode: false,
        pending_operations: Vec::new(),
    };
    
    // Perform sync
    let report = sync_engine.sync().await?;
    
    println!("Sync completed:");
    println!("  Files uploaded: {}", report.files_uploaded);
    println!("  Files downloaded: {}", report.files_downloaded);
    println!("  Conflicts detected: {}", report.conflicts_detected);
    println!("  Conflicts auto-resolved: {}", report.conflicts_resolved);
    
    if report.offline_mode {
        println!("  Operating in offline mode");
        println!("  Pending operations: {}", report.pending_operations);
    }
    
    Ok(())
}
