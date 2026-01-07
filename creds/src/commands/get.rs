// Updated src/commands/get.rs
use crate::commands::Command;
use crate::Vault;

pub struct GetCommand {
    pub key: String,
    pub show_metadata: bool,
}

impl Command for GetCommand {
    fn execute(&self, vault: &Vault) -> anyhow::Result<()> {
        let value = vault.get(&self.key)?;
        
        if self.show_metadata {
            let metadata = vault.get_metadata(&self.key)?;
            println!("Key: {}", self.key);
            println!("Value: {}", value);
            
            // Convert timestamps to readable format
            let created = chrono::DateTime::from_timestamp(metadata.created_at as i64, 0)
                .unwrap_or_default();
            let updated = chrono::DateTime::from_timestamp(metadata.updated_at as i64, 0)
                .unwrap_or_default();
            
            println!("Created: {}", created.format("%Y-%m-%d %H:%M:%S"));
            println!("Updated: {}", updated.format("%Y-%m-%d %H:%M:%S"));
            
            if let Some(desc) = metadata.description {
                println!("Description: {}", desc);
            }
        } else {
            println!("{}", value);
        }
        
        Ok(())
    }
}
