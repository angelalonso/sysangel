import yaml
import os
from pathlib import Path
import tkinter.messagebox as messagebox

class ConfigManager:
    """Manages application configuration"""
    
    def __init__(self):
        self.cfg_path = "cfg.yml"
        self.gtr2_path = None
        self.teams_path = None
        
    def load_configuration(self):
        """Load configuration from cfg.yml file"""
        try:
            if not os.path.exists(self.cfg_path):
                self.create_default_config()
                messagebox.showinfo("Config Created", 
                                  f"Default config file '{self.cfg_path}' created.\nPlease edit it with your GTR2 installation path.")
                return False
            
            with open(self.cfg_path, 'r') as file:
                config = yaml.safe_load(file)
            
            self.gtr2_path = config.get('gtr2_install_path', '')
            
            if not self.gtr2_path:
                messagebox.showerror("Config Error", 
                                   f"Please set 'gtr2_install_path' in '{self.cfg_path}'")
                return False
            
            # Construct teams path
            self.teams_path = os.path.join(self.gtr2_path, "GameData", "Teams")
            
            if not os.path.exists(self.teams_path):
                messagebox.showwarning("Path Warning", 
                                      f"Teams folder not found at:\n{self.teams_path}\n\nPlease check your GTR2 installation path.")
                return False
                
            return True
            
        except yaml.YAMLError as e:
            messagebox.showerror("Config Error", f"Error reading config file: {e}")
            return False
        except Exception as e:
            messagebox.showerror("Error", f"Unexpected error: {e}")
            return False
    
    def create_default_config(self):
        """Create a default configuration file"""
        default_config = {
            'gtr2_install_path': 'C:/Program Files (x86)/GTR2',
            'note': 'Set the path to your GTR2 installation directory'
        }
        
        with open(self.cfg_path, 'w') as file:
            yaml.dump(default_config, file, default_flow_style=False)
    
    def get_gtr2_path(self):
        """Get GTR2 installation path"""
        return self.gtr2_path
    
    def get_teams_path(self):
        """Get Teams folder path"""
        return self.teams_path
