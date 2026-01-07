use crate::commands::Command;
use crate::Vault;

pub struct DeleteCommand {
    pub key: String,
    pub force: bool,
}

impl Command for DeleteCommand {
    fn execute(&self, vault: &Vault) -> anyhow::Result<()> {
        if !self.force {
            println!("Are you sure you want to delete '{}'? (y/N): ", self.key);
            let mut confirm = String::new();
            std::io::stdin().read_line(&mut confirm)?;
            
            if !confirm.trim().eq_ignore_ascii_case("y") {
                println!("Cancelled.");
                return Ok(());
            }
        }
        
        vault.delete(&self.key)?;
        println!("✓ Deleted '{}'", self.key);
        Ok(())
    }
}
