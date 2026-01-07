use crate::commands::Command;
use crate::Vault;

pub struct ListCommand {
    pub show_descriptions: bool,
}

impl Command for ListCommand {
    fn execute(&self, vault: &Vault) -> anyhow::Result<()> {
        let secrets = vault.list()?;
        
        if secrets.is_empty() {
            println!("No secrets stored.");
            return Ok(());
        }
        
        println!("Stored secrets ({} total):", secrets.len());
        println!();
        
        for key in secrets {
            if self.show_descriptions {
                if let Ok(metadata) = vault.get_metadata(&key) {
                    let desc = metadata.description.unwrap_or_else(|| "No description".to_string());
                    println!("  • {} - {}", key, desc);
                } else {
                    println!("  • {}", key);
                }
            } else {
                println!("  • {}", key);
            }
        }
        
        Ok(())
    }
}
