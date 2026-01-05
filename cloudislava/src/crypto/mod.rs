// src/crypto/mod.rs
pub mod encryption;
pub mod integrity;

use hmac::{Hmac, Mac};
use sha2::Sha256;
use zeroize::Zeroize;
use getrandom::getrandom;
use thiserror::Error;

#[derive(Error, Debug)]
pub enum CryptoError {
    #[error("Random number generation failed")]
    Random(String),
    #[error("Encryption error: {0}")]
    Encryption(String),
    #[error("Decryption error: {0}")]
    Decryption(String),
}

pub struct CryptoManager {
    master_key: Vec<u8>,
}

impl CryptoManager {
    pub fn new(master_key: &[u8]) -> Self {
        Self {
            master_key: master_key.to_vec(),
        }
    }
    
    pub fn derive_file_key(&self, file_id: &str) -> Vec<u8> {
        let mut mac = Hmac::<Sha256>::new_from_slice(&self.master_key)
            .expect("HMAC can take key of any size");
        mac.update(file_id.as_bytes());
        mac.finalize().into_bytes().to_vec()
    }
    
    pub fn generate_nonce(&self) -> Result<[u8; 12], CryptoError> {
        let mut nonce = [0u8; 12];
        getrandom(&mut nonce)
            .map_err(|e| CryptoError::Random(format!("Failed to generate random bytes: {}", e)))?;
        Ok(nonce)
    }
    
    pub fn clear_master_key(&mut self) {
        self.master_key.zeroize();
    }
}
