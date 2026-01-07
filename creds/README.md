# creds - Simple Credential Manager

A minimal, secure credential manager for developers.

## Features

- 🔒 AES-256-GCM encryption
- 📁 Local storage (no cloud dependency)
- 🎯 Simple CLI interface
- 📝 Secret descriptions/metadata
- 📤 JSON export for backups

## Installation

```bash
# From source
cargo install --path .

# Or from crates.io (when published)
# cargo install creds


## Usage
### Store a secret
bash

# With value as argument
creds set DATABASE_URL "postgres://user:pass@localhost"

# With description
creds set API_KEY "sk_..." --description "Stripe production key"

# Prompt for value (hidden input)
creds set GITHUB_TOKEN

### Retrieve a secret
bash

# Just the value
creds get DATABASE_URL

# With metadata
creds get DATABASE_URL --metadata

### List secrets
bash

# List all keys
creds list

# With descriptions
creds list --descriptions

### Delete a secret
bash

creds delete OLD_KEY

# Skip confirmation
creds delete OLD_KEY --force

### Export all secrets
bash

# To stdout
creds export

# To file
creds export --output backup.json

## Vault Location

By default, vault is stored at:

    Linux/macOS: ~/.config/creds/vault.dat

    Windows: %APPDATA%\creds\vault.dat

Use --vault flag to specify custom location:
bash

creds --vault ./my-vault.dat list

## Security Notes

⚠️ This is for personal/development use only!

    Uses simple key derivation (not suitable for production)

    No key rotation

    No audit logging

    Master password must be strong!

## Development
bash

# Build
cargo build

# Run tests
cargo test

# Install locally
cargo install --path .
