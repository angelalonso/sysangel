// src/crypto/integrity.rs
use sha2::{Sha256, Digest};
use hmac::{Hmac, Mac};
use serde::{Serialize, Deserialize};

pub struct IntegrityVerifier;

impl IntegrityVerifier {
    /// Create a digital signature for data
    pub fn sign(data: &[u8], key: &[u8], file_id: &str, version: u64) -> String {
        let mut mac = Hmac::<Sha256>::new_from_slice(key)
            .expect("HMAC can take key of any size");
        
        // Include file_id and version in signature to prevent replay attacks
        mac.update(file_id.as_bytes());
        mac.update(&version.to_be_bytes());
        mac.update(data);
        
        let result = mac.finalize();
        hex::encode(result.into_bytes())
    }
    
    /// Verify digital signature
    pub fn verify(data: &[u8], key: &[u8], file_id: &str, version: u64, signature: &str) -> bool {
        let computed = Self::sign(data, key, file_id, version);
        computed == signature
    }
    
    /// Create content hash (for deduplication and integrity)
    pub fn content_hash(data: &[u8]) -> String {
        let mut hasher = Sha256::new();
        hasher.update(data);
        hex::encode(hasher.finalize())
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct FileSignature {
    pub file_id: String,
    pub version: u64,
    pub signature: String,
    pub content_hash: String,
    pub timestamp: chrono::DateTime<chrono::Utc>,
    pub author: String, // Machine/device identifier
}
