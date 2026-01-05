// src/dropbox/client.rs
use reqwest::Client;
use serde::{Deserialize, Serialize};
use bytes::Bytes;
use thiserror::Error;
use std::time::Duration;

const DROPBOX_API_BASE: &str = "https://api.dropboxapi.com/2";
const DROPBOX_CONTENT_BASE: &str = "https://content.dropboxapi.com/2";

#[derive(Error, Debug)]
pub enum DropboxError {
    #[error("API error: {0}")]
    Api(String),
    #[error("Network error: {0}")]
    Network(#[from] reqwest::Error),
    #[error("Serialization error: {0}")]
    Serialization(#[from] serde_json::Error),
    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),
}

pub struct DropboxClient {
    client: Client,
    access_token: String,
    base_path: String,
}

impl DropboxClient {
    pub fn new(access_token: String, base_path: Option<String>) -> Self {
        let client = Client::builder()
            .timeout(Duration::from_secs(30))
            .build()
            .expect("Failed to create HTTP client");
        
        Self {
            client,
            access_token,
            base_path: base_path.unwrap_or_else(|| "/CloudIsLava".to_string()),
        }
    }

    fn auth_header(&self) -> String {
        format!("Bearer {}", self.access_token)
    }

    pub async fn upload_file(
        &self,
        local_path: &std::path::Path,
        remote_filename: &str,
    ) -> Result<String, DropboxError> {
        let remote_path = format!("{}/{}", self.base_path, remote_filename);
        
        // Read file
        let file_data = tokio::fs::read(local_path).await?;
        
        // Prepare upload arguments
        let args = UploadArgs {
            path: remote_path.clone(),
            mode: "overwrite".to_string(),
            autorename: false,
            mute: true,
        };
        
        let args_json = serde_json::to_string(&args)?;
        
        // Upload to Dropbox
        let response = self.client
            .post(format!("{}/files/upload", DROPBOX_CONTENT_BASE))
            .header("Authorization", self.auth_header())
            .header("Dropbox-API-Arg", &args_json)
            .header("Content-Type", "application/octet-stream")
            .body(file_data)
            .send()
            .await?;
        
        if !response.status().is_success() {
            let error_text = response.text().await.unwrap_or_default();
            return Err(DropboxError::Api(format!("Upload failed: {}", error_text)));
        }
        
        let result: UploadResult = response.json().await?;
        Ok(result.id)
    }

    pub async fn download_file(&self, remote_filename: &str) -> Result<Bytes, DropboxError> {
        let remote_path = format!("{}/{}", self.base_path, remote_filename);
        
        let args = DownloadArgs {
            path: remote_path,
        };
        
        let args_json = serde_json::to_string(&args)?;
        
        let response = self.client
            .post(format!("{}/files/download", DROPBOX_CONTENT_BASE))
            .header("Authorization", self.auth_header())
            .header("Dropbox-API-Arg", &args_json)
            .send()
            .await?;
        
        if !response.status().is_success() {
            let error_text = response.text().await.unwrap_or_default();
            return Err(DropboxError::Api(format!("Download failed: {}", error_text)));
        }
        
        let data = response.bytes().await?;
        Ok(data)
    }

    pub async fn list_files(&self) -> Result<Vec<RemoteFile>, DropboxError> {
        let args = ListFolderArgs {
            path: self.base_path.clone(),
            recursive: true,
            include_media_info: false,
            include_deleted: false,
            include_has_explicit_shared_members: false,
            include_mounted_folders: true,
            limit: None,
            shared_link: None,
            include_property_groups: None,
            include_non_downloadable_files: false,
        };
        
        let response = self.client
            .post(format!("{}/files/list_folder", DROPBOX_API_BASE))
            .header("Authorization", self.auth_header())
            .header("Content-Type", "application/json")
            .json(&args)
            .send()
            .await?;
        
        if !response.status().is_success() {
            let error_text = response.text().await.unwrap_or_default();
            return Err(DropboxError::Api(format!("List failed: {}", error_text)));
        }
        
        let result: ListFolderResult = response.json().await?;
        
        let mut files = Vec::new();
        for entry in result.entries {
            if let Metadata::File(file_metadata) = entry {
                let modified = chrono::DateTime::parse_from_rfc3339(&file_metadata.client_modified)
                    .unwrap_or_else(|_| chrono::Utc::now().into())
                    .with_timezone(&chrono::Utc);
                    
                files.push(RemoteFile {
                    path: file_metadata.path_display.unwrap_or_default(),
                    id: file_metadata.id,
                    size: file_metadata.size as u64,
                    modified,
                });
            }
        }
        
        Ok(files)
    }

    pub async fn delete_file(&self, remote_filename: &str) -> Result<(), DropboxError> {
        let remote_path = format!("{}/{}", self.base_path, remote_filename);
        
        let args = DeleteArgs {
            path: remote_path,
        };
        
        let response = self.client
            .post(format!("{}/files/delete_v2", DROPBOX_API_BASE))
            .header("Authorization", self.auth_header())
            .header("Content-Type", "application/json")
            .json(&args)
            .send()
            .await?;
        
        if !response.status().is_success() {
            let error_text = response.text().await.unwrap_or_default();
            return Err(DropboxError::Api(format!("Delete failed: {}", error_text)));
        }
        
        Ok(())
    }
}

#[derive(Debug, Clone)]
pub struct RemoteFile {
    pub path: String,
    pub id: String,
    pub size: u64,
    pub modified: chrono::DateTime<chrono::Utc>,
}

// Dropbox API structures
#[derive(Serialize)]
struct UploadArgs {
    path: String,
    mode: String,
    autorename: bool,
    mute: bool,
}

#[derive(Deserialize)]
struct UploadResult {
    id: String,
    name: String,
    path_lower: String,
    size: u64,
}

#[derive(Serialize)]
struct DownloadArgs {
    path: String,
}

#[derive(Serialize)]
struct ListFolderArgs {
    path: String,
    recursive: bool,
    include_media_info: bool,
    include_deleted: bool,
    include_has_explicit_shared_members: bool,
    include_mounted_folders: bool,
    limit: Option<u32>,
    shared_link: Option<String>,
    include_property_groups: Option<serde_json::Value>,
    include_non_downloadable_files: bool,
}

#[derive(Deserialize)]
struct ListFolderResult {
    entries: Vec<Metadata>,
    cursor: String,
    has_more: bool,
}

#[derive(Deserialize)]
#[serde(tag = ".tag")]
enum Metadata {
    #[serde(rename = "file")]
    File(FileMetadata),
    #[serde(rename = "folder")]
    Folder(FolderMetadata),
}

#[derive(Deserialize)]
struct FileMetadata {
    name: String,
    path_lower: String,
    path_display: Option<String>,
    id: String,
    client_modified: String,
    server_modified: String,
    rev: String,
    size: u64,
    is_downloadable: bool,
    content_hash: String,
}

#[derive(Deserialize)]
struct FolderMetadata {
    name: String,
    path_lower: String,
    path_display: Option<String>,
    id: String,
}

#[derive(Serialize)]
struct DeleteArgs {
    path: String,
}
