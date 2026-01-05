// src/crypto/encryption.rs
use super::CryptoManager;
use aes_gcm::{Aes256Gcm, Key, Nonce};
use aes_gcm::aead::{Aead, KeyInit};
use base64::{Engine as _, engine::general_purpose};
use anyhow::anyhow;
use serde::{Serialize, Deserialize};

pub struct FileEncryptor;

impl FileEncryptor {
    /// Encrypt file data with authenticated encryption
    pub fn encrypt(
        data: &[u8],
        crypto: &CryptoManager,
        file_id: &str,
    ) -> Result<EncryptedData, anyhow::Error> {
        // Derive file-specific key
        let file_key = crypto.derive_file_key(file_id);
        
        // Generate nonce
        let nonce_bytes = crypto.generate_nonce()
            .map_err(|e| anyhow!("Failed to generate nonce: {}", e))?;
        
        // Create cipher
        let key = Key::<Aes256Gcm>::from_slice(&file_key);
        let cipher = Aes256Gcm::new(key);
        let nonce = Nonce::from_slice(&nonce_bytes);
        
        // Encrypt data
        let ciphertext = cipher.encrypt(nonce, data)
            .map_err(|e| anyhow!("Encryption failed: {}", e))?;
        
        // Create metadata
        let metadata = EncryptionMetadata {
            file_id: file_id.to_string(),
            nonce: general_purpose::STANDARD.encode(nonce_bytes),
            algorithm: "AES-256-GCM".to_string(),
            version: 1,
        };
        
        Ok(EncryptedData {
            ciphertext,
            metadata,
        })
    }
    
    /// Decrypt file data
    pub fn decrypt(
        encrypted: &EncryptedData,
        crypto: &CryptoManager,
    ) -> Result<Vec<u8>, anyhow::Error> {
        let file_key = crypto.derive_file_key(&encrypted.metadata.file_id);
        
        let nonce_bytes = general_purpose::STANDARD.decode(&encrypted.metadata.nonce)?;
        
        let key = Key::<Aes256Gcm>::from_slice(&file_key);
        let cipher = Aes256Gcm::new(key);
        let nonce = Nonce::from_slice(&nonce_bytes);
        
        cipher.decrypt(nonce, encrypted.ciphertext.as_ref())
            .map_err(|e| anyhow!("Decryption failed: {}", e))
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EncryptionMetadata {
    pub file_id: String,
    pub nonce: String,
    pub algorithm: String,
    pub version: u8,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct EncryptedData {
    pub ciphertext: Vec<u8>,
    pub metadata: EncryptionMetadata,
}
