// src/local/db.rs
use rusqlite::{Connection, params};
use anyhow::Context;

pub struct LocalDatabase {
    conn: Connection,
}

impl LocalDatabase {
    pub fn new(path: &std::path::Path) -> anyhow::Result<Self> {
        let conn = Connection::open(path)
            .context("Failed to open database")?;
        
        // Create tables
        conn.execute_batch(
            r#"
            CREATE TABLE IF NOT EXISTS files (
                id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                local_path TEXT NOT NULL,
                remote_path TEXT,
                size INTEGER NOT NULL,
                version INTEGER NOT NULL DEFAULT 1,
                content_hash TEXT NOT NULL,
                created_at TEXT NOT NULL,
                modified_at TEXT NOT NULL,
                last_sync_at TEXT,
                metadata TEXT NOT NULL,
                sync_status TEXT NOT NULL DEFAULT 'local'
            );
            
            CREATE TABLE IF NOT EXISTS sync_history (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                file_id TEXT NOT NULL,
                operation TEXT NOT NULL,
                timestamp TEXT NOT NULL,
                success BOOLEAN NOT NULL,
                error_message TEXT,
                FOREIGN KEY (file_id) REFERENCES files (id)
            );
            
            CREATE TABLE IF NOT EXISTS pending_operations (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                file_id TEXT NOT NULL,
                operation TEXT NOT NULL,
                data TEXT NOT NULL,
                created_at TEXT NOT NULL,
                attempted BOOLEAN NOT NULL DEFAULT FALSE,
                FOREIGN KEY (file_id) REFERENCES files (id)
            );
            
            CREATE INDEX IF NOT EXISTS idx_files_sync_status ON files (sync_status);
            CREATE INDEX IF NOT EXISTS idx_pending_operations_created ON pending_operations (created_at);
            "#,
        )?;
        
        Ok(Self { conn })
    }
    
    pub fn add_file(&self, metadata: &crate::models::FileMetadata, local_path: &str) -> anyhow::Result<()> {
        let metadata_json = serde_json::to_string(metadata)?;
        
        self.conn.execute(
            r#"
            INSERT OR REPLACE INTO files 
            (id, name, local_path, size, version, content_hash, created_at, modified_at, metadata, sync_status)
            VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, 'local')
            "#,
            params![
                &metadata.id,
                &metadata.name,
                local_path,
                metadata.size as i64,
                metadata.version as i64,
                &metadata.content_hash,
                &metadata.created_at.to_rfc3339(),
                &metadata.modified_at.to_rfc3339(),
                &metadata_json,
            ],
        )?;
        
        Ok(())
    }
    
    pub fn get_files(&self) -> anyhow::Result<Vec<FileRecord>> {
        let mut stmt = self.conn.prepare(
            "SELECT id, name, local_path, sync_status, version FROM files ORDER BY name"
        )?;
        
        let rows = stmt.query_map([], |row| {
            Ok(FileRecord {
                id: row.get(0)?,
                name: row.get(1)?,
                local_path: row.get(2)?,
                sync_status: row.get(3)?,
                version: row.get(4)?,
            })
        })?;
        
        let mut files = Vec::new();
        for row in rows {
            files.push(row?);
        }
        
        Ok(files)
    }
}

pub struct FileRecord {
    pub id: String,
    pub name: String,
    pub local_path: String,
    pub sync_status: String,
    pub version: i64,
}
