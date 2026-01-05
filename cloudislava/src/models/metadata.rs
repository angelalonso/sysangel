// src/models/metadata.rs
use serde::{Serialize, Deserialize};
use chrono::{DateTime, Utc};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FileMetadata {
    pub id: String,
    pub name: String,
    pub size: u64,
    pub version: u64,
    pub content_hash: String,
    pub created_at: DateTime<Utc>,
    pub modified_at: DateTime<Utc>,
    pub last_sync_at: Option<DateTime<Utc>>,
    pub signatures: Vec<crate::crypto::integrity::FileSignature>,
    pub parent_id: Option<String>, // For directory structure
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SyncMetadata {
    pub device_id: String,
    pub last_sync_time: DateTime<Utc>,
    pub sync_token: String, // For incremental sync
    pub pending_operations: Vec<SyncOperation>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum SyncOperation {
    Create(FileMetadata),
    Update(FileMetadata),
    Delete { file_id: String, timestamp: DateTime<Utc> },
}
