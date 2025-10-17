import yaml
import os
from typing import Any, Dict, Optional
import logging

class ConfigManager:
    def __init__(self, config_dir: str = "cfg_data", config_file: str = "app_config.yaml"):
        self.config_dir = config_dir
        self.config_file = config_file
        self.config_path = os.path.join(config_dir, config_file)
        self.logger = self._setup_logging()
        self.ensure_config_directory()
        self.default_config = self._get_default_config()
        self.config = self.load_config()
    
    def _setup_logging(self):
        """Setup logging for config manager"""
        logging.basicConfig(level=logging.INFO)
        return logging.getLogger(__name__)
    
    def ensure_config_directory(self):
        """Ensure the config directory exists"""
        try:
            os.makedirs(self.config_dir, exist_ok=True)
            self.logger.info(f"Config directory ensured: {self.config_dir}")
        except Exception as e:
            self.logger.error(f"Failed to create config directory: {e}")
            raise
    
    def _get_default_config(self) -> Dict[str, Any]:
        """Return default configuration"""
        return {
            'appearance': {
                'mode': 'System',
                'theme': 'blue'
            },
            'application': {
                'auto_save': True,
                'notifications': True,
                'language': 'en'
            },
            'backup': {
                'compression': True,
                'encryption': False,
                'default_type': 'full',
                'media': []  # List for configured backup media
            },
            'restore': {
                'verification': True,
                'create_backup_before_restore': True
            }
        }
    
    def load_config(self) -> Dict[str, Any]:
        """Load configuration from YAML file, create with defaults if not exists"""
        try:
            if os.path.exists(self.config_path):
                with open(self.config_path, 'r', encoding='utf-8') as f:
                    loaded_config = yaml.safe_load(f) or {}
                
                # Merge with defaults to ensure all keys exist
                config = self._merge_configs(self.default_config, loaded_config)
                self.logger.info("Configuration loaded successfully")
                return config
            else:
                self.logger.info("No config file found, creating with defaults")
                return self.create_default_config()
                
        except Exception as e:
            self.logger.error(f"Error loading config: {e}")
            return self.default_config.copy()
    
    def _merge_configs(self, default: Dict, loaded: Dict) -> Dict:
        """Recursively merge loaded config with defaults"""
        merged = default.copy()
        
        for key, value in loaded.items():
            if key in merged:
                if isinstance(merged[key], dict) and isinstance(value, dict):
                    merged[key] = self._merge_configs(merged[key], value)
                else:
                    merged[key] = value
            else:
                merged[key] = value
        
        return merged
    
    def create_default_config(self) -> Dict[str, Any]:
        """Create default configuration file"""
        try:
            self.save_config(self.default_config)
            return self.default_config.copy()
        except Exception as e:
            self.logger.error(f"Failed to create default config: {e}")
            return self.default_config.copy()
    
    def save_config(self, config: Dict[str, Any] = None):
        """Save configuration to YAML file"""
        try:
            if config is None:
                config = self.config
            
            with open(self.config_path, 'w', encoding='utf-8') as f:
                yaml.dump(config, f, default_flow_style=False, indent=2, allow_unicode=True)
            
            self.logger.info("Configuration saved successfully")
        except Exception as e:
            self.logger.error(f"Error saving config: {e}")
            raise
    
    def get(self, key_path: str, default: Any = None) -> Any:
        """Get configuration value using dot notation (e.g., 'appearance.mode')"""
        try:
            keys = key_path.split('.')
            value = self.config
            
            for key in keys:
                if isinstance(value, dict) and key in value:
                    value = value[key]
                else:
                    return default
            
            return value
        except (AttributeError, KeyError, TypeError):
            return default
    
    def set(self, key_path: str, value: Any, save: bool = True):
        """Set configuration value using dot notation"""
        try:
            keys = key_path.split('.')
            config = self.config
            
            # Navigate to the parent of the final key
            for key in keys[:-1]:
                if key not in config or not isinstance(config[key], dict):
                    config[key] = {}
                config = config[key]
            
            # Set the final key
            config[keys[-1]] = value
            
            if save:
                self.save_config()
                
            self.logger.debug(f"Config set: {key_path} = {value}")
        except Exception as e:
            self.logger.error(f"Error setting config {key_path}: {e}")
            raise
    
    def get_all_config(self) -> Dict[str, Any]:
        """Get entire configuration as a dictionary"""
        return self.config.copy()
    
    def export_config(self, file_path: str = None):
        """Export configuration to specified YAML file"""
        if file_path is None:
            file_path = os.path.join(self.config_dir, "app_config_export.yaml")
        
        try:
            with open(file_path, 'w', encoding='utf-8') as f:
                yaml.dump(self.config, f, default_flow_style=False, indent=2, allow_unicode=True)
            
            self.logger.info(f"Configuration exported to {file_path}")
            return file_path
        except Exception as e:
            self.logger.error(f"Error exporting config: {e}")
            raise
    
    def import_config(self, file_path: str):
        """Import configuration from YAML file"""
        try:
            if not os.path.exists(file_path):
                raise FileNotFoundError(f"Config file not found: {file_path}")
            
            with open(file_path, 'r', encoding='utf-8') as f:
                imported_config = yaml.safe_load(f)
            
            if not isinstance(imported_config, dict):
                raise ValueError("Invalid config file format")
            
            # Merge imported config with current config
            self.config = self._merge_configs(self.config, imported_config)
            self.save_config()
            
            self.logger.info(f"Configuration imported from {file_path}")
        except Exception as e:
            self.logger.error(f"Error importing config: {e}")
            raise
    
    def reset_to_defaults(self):
        """Reset configuration to defaults"""
        self.config = self.default_config.copy()
        self.save_config()
        self.logger.info("Configuration reset to defaults")

# Global instance
config_manager = ConfigManager()
