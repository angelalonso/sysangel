use crate::commands::Command;
use crate::Vault;
use std::fs;

pub struct ExportCommand {
    pub output: Option<String>,
}

impl Command for ExportCommand {
    fn execute(&self, vault: &Vault) -> anyhow::Result<()> {
        let secrets = vault.export()?;
        
        let json = serde_json::to_string_pretty(&secrets)?;
        
        match &self.output {
            Some(path) => {
                fs::write(path, json)?;
                println!("✓ Exported {} secrets to {}", secrets.len(), path);
            }
            None => {
                println!("{}", json);
            }
        }
        
        Ok(())
    }
}
