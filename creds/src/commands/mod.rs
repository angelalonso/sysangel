mod get;
mod set;
mod list;
mod delete;
mod export;

pub use get::GetCommand;
pub use set::SetCommand;
pub use list::ListCommand;
pub use delete::DeleteCommand;
pub use export::ExportCommand;

use crate::Vault;
use std::path::PathBuf;

pub trait Command {
    fn execute(&self, vault: &Vault) -> anyhow::Result<()>;
}

fn get_default_vault_path() -> PathBuf {
    dirs::config_dir()
        .expect("Could not find config directory")
        .join("creds")
        .join("vault.dat")
}

pub fn create_vault() -> anyhow::Result<Vault> {
    println!("Enter master password: ");
    let password = rpassword::read_password()?;
    
    let vault_path = get_default_vault_path();
    if vault_path.exists() {
        println!("Using existing vault at: {}", vault_path.display());
    } else {
        println!("Creating new vault at: {}", vault_path.display());
    }
    
    Ok(Vault::with_path(&password, Some(&vault_path))?)
}
