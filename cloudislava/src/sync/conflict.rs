// src/sync/conflict.rs
use super::SyncState;
use crate::models::metadata::FileMetadata;

#[derive(Debug)]
pub enum ConflictResolution {
    KeepLocal,
    KeepRemote,
    Merge { merged_content: Vec<u8> },
    RenameLocal { new_name: String },
    RenameRemote { new_name: String },
}

pub struct ConflictResolver;

impl ConflictResolver {
    /// Detect conflicts between local and remote versions
    pub fn detect_conflict(
        local: &FileMetadata,
        remote: &FileMetadata,
        sync_state: &SyncState,
    ) -> bool {
        // If versions diverged from common ancestor
        if local.version != remote.version {
            // Check if both have been modified since last sync
            let local_modified_after_base = local.timestamp > sync_state.last_sync;
            let remote_modified_after_base = remote.timestamp > sync_state.last_sync;
            
            if local_modified_after_base && remote_modified_after_base {
                // Check if content actually differs (not just metadata)
                if local.content_hash != remote.content_hash {
                    return true;
                }
            }
        }
        false
    }
    
    /// Attempt automatic conflict resolution
    pub fn auto_resolve(
        local: &FileMetadata,
        remote: &FileMetadata,
    ) -> Option<ConflictResolution> {
        // Simple automatic resolution: keep the most recent version
        if local.timestamp > remote.timestamp {
            Some(ConflictResolution::KeepLocal)
        } else {
            Some(ConflictResolution::KeepRemote)
        }
    }
    
    /// Three-way merge attempt (simplified)
    pub fn three_way_merge(
        base_content: &[u8],
        local_content: &[u8],
        remote_content: &[u8],
    ) -> Option<Vec<u8>> {
        // This is a simplified version
        // In a real implementation, you'd use proper diff/merge algorithms
        
        // If one side hasn't changed, take the other
        if base_content == local_content {
            return Some(remote_content.to_vec());
        }
        if base_content == remote_content {
            return Some(local_content.to_vec());
        }
        
        // If both changed the same way
        if local_content == remote_content {
            return Some(local_content.to_vec());
        }
        
        // Can't auto-merge
        None
    }
}
