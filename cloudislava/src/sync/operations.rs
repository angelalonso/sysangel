// src/sync/operations.rs
use super::{SyncState, SyncStatus};
use crate::models::metadata::FileMetadata;
use crate::crypto::integrity::FileSignature;
use crate::local::db::LocalDatabase;
use std::path::PathBuf;

pub struct SyncEngine {
    local_db: LocalDatabase,
    dropbox_client: crate::dropbox::client::DropboxClient,
    offline_mode: bool,
    pending_operations: Vec<SyncOperation>,
}

#[derive(Debug)]
enum SyncOperation {
    Upload {
        file_id: String,
        local_path: PathBuf,
        metadata: FileMetadata,
    },
    Download {
        file_id: String,
        remote_path: String,
        local_path: PathBuf,
    },
    Delete {
        file_id: String,
        is_local: bool,
        is_remote: bool,
    },
    ResolveConflict {
        file_id: String,
        resolution: super::conflict::ConflictResolution,
    },
}

impl SyncEngine {
    pub async fn sync(&mut self) -> anyhow::Result<SyncReport> {
        let mut report = SyncReport::new();
        
        if self.offline_mode {
            log::warn!("Operating in offline mode - only local operations available");
            // Queue operations for when connectivity is restored
            return self.queue_offline_operations().await;
        }
        
        // 1. Check connectivity
        if !self.check_connectivity().await {
            self.offline_mode = true;
            return self.queue_offline_operations().await;
        }
        
        // 2. Get remote state
        let remote_state = self.dropbox_client.list_files().await?;
        
        // 3. Compare with local state
        let local_state = self.local_db.get_all_files()?;
        
        // 4. Detect and resolve conflicts
        let conflicts = self.detect_conflicts(&local_state, &remote_state).await?;
        report.conflicts_detected = conflicts.len();
        
        for conflict in conflicts {
            // Try auto-resolution first
            if let Some(resolution) = self.auto_resolve_conflict(&conflict).await? {
                self.apply_resolution(conflict.file_id, resolution).await?;
                report.conflicts_resolved += 1;
            } else {
                // Manual resolution needed
                report.conflicts_need_manual += 1;
                self.queue_conflict_for_manual_resolution(conflict).await?;
            }
        }
        
        // 5. Apply non-conflicting changes
        self.apply_sync_operations().await?;
        
        // 6. Update sync timestamps
        self.update_sync_timestamps().await?;
        
        Ok(report)
    }
    
    async fn queue_offline_operations(&mut self) -> anyhow::Result<SyncReport> {
        let mut report = SyncReport::new();
        report.offline_mode = true;
        
        // Store local changes for later sync
        let pending_changes = self.local_db.get_pending_changes()?;
        report.pending_operations = pending_changes.len();
        
        // Log changes for when connectivity is restored
        for change in pending_changes {
            self.pending_operations.push(SyncOperation::Upload {
                file_id: change.file_id,
                local_path: change.local_path,
                metadata: change.metadata,
            });
            log::info!("Queued operation for file {} (offline)", change.file_id);
        }
        
        Ok(report)
    }
}

pub struct SyncReport {
    pub files_uploaded: usize,
    pub files_downloaded: usize,
    pub conflicts_detected: usize,
    pub conflicts_resolved: usize,
    pub conflicts_need_manual: usize,
    pub pending_operations: usize,
    pub offline_mode: bool,
}
