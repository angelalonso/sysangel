import customtkinter as ctk
import tkinter as tk
from tkinter import filedialog, messagebox
import yaml
import os
from pathlib import Path

ctk.set_appearance_mode("System")
ctk.set_default_color_theme("blue")

class GTR2TalentsTuner(ctk.CTk):
    def __init__(self):
        super().__init__()
        
        self.title("GTR2 Talents Tuner")
        self.geometry("900x700")
        
        # Configuration
        self.cfg_path = "cfg.yml"
        self.gtr2_path = None
        self.teams_path = None
        
        # Selected folder
        self.selected_folder = tk.StringVar()
        self.selected_folder.set("No folder selected")
        
        # Try to load configuration
        if not self.load_configuration():
            return  # Configuration failed
        
        # Build UI
        self.setup_ui()
        
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
    
    def setup_ui(self):
        """Setup the user interface"""
        # Create main container with padding
        main_container = ctk.CTkFrame(self)
        main_container.pack(fill="both", expand=True, padx=20, pady=20)
        
        # Title
        title_label = ctk.CTkLabel(
            main_container,
            text="GTR2 Talents Tuner",
            font=("Arial", 28, "bold")
        )
        title_label.pack(pady=(0, 20))
        
        # Configuration info frame
        config_frame = ctk.CTkFrame(main_container)
        config_frame.pack(fill="x", pady=(0, 20))
        
        config_title = ctk.CTkLabel(
            config_frame,
            text="Configuration",
            font=("Arial", 16, "bold")
        )
        config_title.pack(pady=(10, 5))
        
        # Show paths
        path_info = f"GTR2 Path: {self.gtr2_path}\nTeams Folder: {self.teams_path}"
        path_label = ctk.CTkLabel(
            config_frame,
            text=path_info,
            font=("Arial", 12),
            wraplength=600,
            justify="left"
        )
        path_label.pack(pady=10, padx=20)
        
        # Folder selection section
        selection_frame = ctk.CTkFrame(main_container)
        selection_frame.pack(fill="both", expand=True)
        
        selection_title = ctk.CTkLabel(
            selection_frame,
            text="Select Team Folder",
            font=("Arial", 18, "bold")
        )
        selection_title.pack(pady=(15, 10))
        
        # Instruction text
        instruction = "Browse to select a team folder from the Teams directory.\nYou can navigate recursively through subdirectories."
        instruction_label = ctk.CTkLabel(
            selection_frame,
            text=instruction,
            font=("Arial", 12),
            wraplength=600,
            justify="center"
        )
        instruction_label.pack(pady=(0, 20))
        
        # Browse button
        browse_button = ctk.CTkButton(
            selection_frame,
            text="Browse Team Folders",
            command=self.browse_folder,
            font=("Arial", 14),
            height=40,
            width=200
        )
        browse_button.pack(pady=20)
        
        # Selected folder display
        selected_frame = ctk.CTkFrame(selection_frame)
        selected_frame.pack(fill="x", padx=50, pady=20)
        
        selected_label = ctk.CTkLabel(
            selected_frame,
            text="Selected Folder:",
            font=("Arial", 14, "bold")
        )
        selected_label.pack(anchor="w", padx=10, pady=(10, 5))
        
        self.selected_display = ctk.CTkLabel(
            selected_frame,
            textvariable=self.selected_folder,
            font=("Arial", 12),
            wraplength=700,
            justify="left",
            anchor="w"
        )
        self.selected_display.pack(fill="x", padx=10, pady=(0, 10))
        
        # Action buttons frame
        action_frame = ctk.CTkFrame(main_container)
        action_frame.pack(fill="x", pady=20)
        
        # Copy path button
        copy_button = ctk.CTkButton(
            action_frame,
            text="Copy Path to Clipboard",
            command=self.copy_to_clipboard,
            font=("Arial", 12),
            height=35,
            fg_color="green",
            hover_color="dark green"
        )
        copy_button.pack(side="left", padx=10, pady=10)
        
        # Open folder button
        open_button = ctk.CTkButton(
            action_frame,
            text="Open Folder",
            command=self.open_folder,
            font=("Arial", 12),
            height=35,
            fg_color="blue",
            hover_color="dark blue"
        )
        open_button.pack(side="left", padx=10, pady=10)
        
        # Reset selection button
        reset_button = ctk.CTkButton(
            action_frame,
            text="Reset Selection",
            command=self.reset_selection,
            font=("Arial", 12),
            height=35,
            fg_color="orange",
            hover_color="dark orange"
        )
        reset_button.pack(side="left", padx=10, pady=10)
        
        # Status bar
        self.status_bar = ctk.CTkLabel(
            main_container,
            text="Ready",
            font=("Arial", 10)
        )
        self.status_bar.pack(fill="x", pady=(10, 0))
    
    def browse_folder(self):
        """Open file browser to select a folder"""
        if not self.teams_path or not os.path.exists(self.teams_path):
            messagebox.showerror("Error", "Teams path not found. Please check configuration.")
            return
        
        try:
            folder_path = filedialog.askdirectory(
                title="Select Team Folder",
                initialdir=self.teams_path,
                mustexist=True
            )
            
            if folder_path:
                # Ensure the selected folder is within the teams path
                if os.path.commonpath([folder_path, self.teams_path]) == self.teams_path:
                    self.selected_folder.set(folder_path)
                    self.update_status(f"Selected: {folder_path}")
                    
                    # Count files in folder for info
                    file_count = len([f for f in os.listdir(folder_path) 
                                    if os.path.isfile(os.path.join(folder_path, f))])
                    
                    messagebox.showinfo(
                        "Folder Selected",
                        f"Successfully selected folder:\n{folder_path}\n\n"
                        f"Contains {file_count} file(s)"
                    )
                else:
                    messagebox.showwarning(
                        "Invalid Selection",
                        "Please select a folder from within the Teams directory."
                    )
            else:
                self.update_status("Folder selection cancelled")
                
        except Exception as e:
            messagebox.showerror("Error", f"Could not browse folder: {e}")
            self.update_status("Error browsing folder")
    
    def copy_to_clipboard(self):
        """Copy selected path to clipboard"""
        path = self.selected_folder.get()
        if path and path != "No folder selected":
            self.clipboard_clear()
            self.clipboard_append(path)
            self.update_status("Path copied to clipboard")
            messagebox.showinfo("Copied", f"Path copied to clipboard:\n{path}")
        else:
            messagebox.showwarning("No Selection", "Please select a folder first.")
    
    def open_folder(self):
        """Open the selected folder in file explorer"""
        path = self.selected_folder.get()
        if path and path != "No folder selected" and os.path.exists(path):
            try:
                os.startfile(path)  # Windows
                self.update_status("Folder opened")
            except:
                try:
                    # Try alternative for other OS (though GTR2 is Windows)
                    import subprocess
                    subprocess.run(['explorer', path])  # Alternative for Windows
                    self.update_status("Folder opened")
                except Exception as e:
                    messagebox.showerror("Error", f"Could not open folder: {e}")
        else:
            messagebox.showwarning("Error", "Selected folder does not exist or no folder selected.")
    
    def reset_selection(self):
        """Reset the folder selection"""
        self.selected_folder.set("No folder selected")
        self.update_status("Selection reset")
        messagebox.showinfo("Reset", "Folder selection has been reset.")
    
    def update_status(self, message):
        """Update status bar message"""
        self.status_bar.configure(text=f"Status: {message}")
    
    def on_closing(self):
        """Handle window closing"""
        self.destroy()

if __name__ == "__main__":
    app = GTR2TalentsTuner()
    app.protocol("WM_DELETE_WINDOW", app.on_closing)
    app.mainloop()
