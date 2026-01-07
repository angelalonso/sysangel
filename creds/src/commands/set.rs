use crate::commands::Command;
use crate::Vault;

pub struct SetCommand {
    pub key: String,
    pub value: Option<String>,
    pub description: Option<String>,
}

impl Command for SetCommand {
    fn execute(&self, vault: &Vault) -> anyhow::Result<()> {
        let value = match &self.value {
            Some(v) => v.clone(),
            None => {
                println!("Enter value for '{}': ", self.key);
                rpassword::read_password()?
            }
        };
        
        vault.set(&self.key, &value, self.description.as_deref())?;
        println!("✓ Saved '{}'", self.key);
        Ok(())
    }
}
