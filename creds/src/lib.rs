pub mod vault;
pub mod error;
pub mod commands;  // Add this line

pub use vault::Vault;
pub use error::Error;

// Re-export for convenience
pub use anyhow::Result;
