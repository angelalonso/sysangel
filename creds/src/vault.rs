use aes_gcm::{
    aead::{Aead, KeyInit, OsRng},
    Aes256Gcm, Key, Nonce
};
use aes_gcm::aead::rand_core::RngCore;
use serde::{Deserialize, Serialize};
use std::{
    collections::HashMap,
    fs, path::{Path, PathBuf},
    time::{SystemTime, UNIX_EPOCH},
};
use dirs;

use crate::error::{Error, Result};

#[derive(Serialize, Deserialize, Debug, Default)]
struct SecretStore {
    version: u32,
    secrets: HashMap<String, Vec<u8>>,
    metadata: HashMap<String, Metadata>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Metadata {
    pub created_at: u64,
    pub updated_at: u64,
    pub description: Option<String>,  // Make all fields public
}

pub struct Vault {
    store_path: PathBuf,
    cipher: Aes256Gcm,
}

impl Vault {
    /// Create a new vault with a master password
    pub fn new(master_password: &str) -> Result<Self> {
        Self::with_path(master_password, None)
    }
    
    /// Create a vault with custom storage path
    pub fn with_path(master_password: &str, custom_path: Option<&Path>) -> Result<Self> {
        // Simple key derivation (in production, use Argon2!)
        let mut key = [0u8; 32];
        let password_bytes = master_password.as_bytes();
        let len = password_bytes.len().min(32);
        key[..len].copy_from_slice(&password_bytes[..len]);
        
        // Fill rest with repeating pattern if password is short
        for i in len..32 {
            key[i] = password_bytes[i % len];
        }
        
        // Determine storage path
        let store_path = if let Some(path) = custom_path {
            path.to_path_buf()
        } else {
            let mut path = dirs::config_dir()
                .ok_or_else(|| Error::InvalidPath("Could not find config directory".to_string()))?;
            path.push("creds");
            path.push("vault.dat");
            path
        };
        
        // Create directory if it doesn't exist
        if let Some(parent) = store_path.parent() {
            fs::create_dir_all(parent)?;
        }
        
        Ok(Self {
            store_path,
            cipher: Aes256Gcm::new(Key::<Aes256Gcm>::from_slice(&key)),
        })
    }
    
    /// Set a secret value
    pub fn set(&self, key: &str, value: &str, description: Option<&str>) -> Result<()> {
        let mut store = self.load()?;
        
        // Generate random nonce (12 bytes for GCM)
        let mut nonce_bytes = [0u8; 12];
        let mut rng = OsRng;
        rng.fill_bytes(&mut nonce_bytes);
        let nonce = Nonce::from_slice(&nonce_bytes);
        
        // Encrypt
        let ciphertext = self.cipher
            .encrypt(nonce, value.as_bytes())
            .map_err(|e| Error::Encryption(e.to_string()))?;
        
        // Combine nonce + ciphertext
        let mut combined = Vec::with_capacity(12 + ciphertext.len());
        combined.extend_from_slice(nonce_bytes.as_slice());
        combined.extend_from_slice(&ciphertext);
        
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_secs();
        
        // Update or create metadata
        if let Some(metadata) = store.metadata.get_mut(key) {
            metadata.updated_at = now;
            if let Some(desc) = description {
                metadata.description = Some(desc.to_string());
            }
        } else {
            store.metadata.insert(key.to_string(), Metadata {
                created_at: now,
                updated_at: now,
                description: description.map(|s| s.to_string()),
            });
        }
        
        store.secrets.insert(key.to_string(), combined);
        self.save(&store)
    }
    
    /// Get a secret value
    pub fn get(&self, key: &str) -> Result<String> {
        let store = self.load()?;
        
        let combined = store.secrets.get(key)
            .ok_or_else(|| Error::NotFound(key.to_string()))?;
        
        if combined.len() < 12 {
            return Err(Error::Decryption("Invalid ciphertext length".to_string()));
        }
        
        let nonce = Nonce::from_slice(&combined[0..12]);
        let ciphertext = &combined[12..];
        
        let plaintext = self.cipher
            .decrypt(nonce, ciphertext)
            .map_err(|e| Error::Decryption(e.to_string()))?;
        
        String::from_utf8(plaintext).map_err(Into::into)
    }
    
    /// Get metadata for a secret
    pub fn get_metadata(&self, key: &str) -> Result<Metadata> {
        let store = self.load()?;
        store.metadata.get(key)
            .cloned()
            .ok_or_else(|| Error::NotFound(key.to_string()))
    }
    
    /// List all secret keys
    pub fn list(&self) -> Result<Vec<String>> {
        let store = self.load()?;
        Ok(store.metadata.keys().cloned().collect())
    }
    
    /// Delete a secret
    pub fn delete(&self, key: &str) -> Result<()> {
        let mut store = self.load()?;
        store.secrets.remove(key);
        store.metadata.remove(key);
        self.save(&store)
    }
    
    /// Export all secrets (for backup)
    pub fn export(&self) -> Result<HashMap<String, String>> {
        let store = self.load()?;
        let mut result = HashMap::new();
        
        for key in store.secrets.keys() {
            if let Ok(value) = self.get(key) {
                result.insert(key.clone(), value);
            }
        }
        
        Ok(result)
    }
    
    /// Check if a secret exists
    pub fn exists(&self, key: &str) -> bool {
        match self.load() {
            Ok(store) => store.secrets.contains_key(key),
            Err(_) => false,
        }
    }
    
    /// Get vault location
    pub fn location(&self) -> &Path {
        &self.store_path
    }
    
    // Private methods
    fn load(&self) -> Result<SecretStore> {
        if !self.store_path.exists() {
            return Ok(SecretStore {
                version: 1,
                ..Default::default()
            });
        }
        
        let data = fs::read(&self.store_path)?;
        bincode::deserialize(&data)
            .map_err(|e| Error::Serialization(e.to_string()))
    }
    
    fn save(&self, store: &SecretStore) -> Result<()> {
        let data = bincode::serialize(store)
            .map_err(|e| Error::Serialization(e.to_string()))?;
        fs::write(&self.store_path, data)?;
        Ok(())
    }
}
