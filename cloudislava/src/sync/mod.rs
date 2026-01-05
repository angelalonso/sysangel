// src/sync/mod.rs
#[derive(Debug, Clone)]
pub enum SyncStatus {
    Synced,
    LocalModified,
    RemoteModified,
    Conflict,
    Offline,
}
