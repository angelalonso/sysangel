import customtkinter as ctk
from tkinter import messagebox, filedialog
import os
import platform
import subprocess
import logging
from config.config_manager import config_manager
from .base_page import BasePage

class ConfigurePage(BasePage):
    def setup_ui(self):
        # Set up logger for this class
        self.logger = logging.getLogger(__name__)
        
        # Header with back button
        self.header_frame = ctk.CTkFrame(self)
        self.header_frame.pack(fill="x", padx=10, pady=10)
        
        self.back_btn = ctk.CTkButton(self.header_frame, 
                                     text="← Back", 
                                     command=lambda: self.controller.show_page("HomePage"),
                                     width=80)
        self.back_btn.pack(side="left", padx=10, pady=10)
        
        self.title_label = ctk.CTkLabel(self.header_frame, 
                                       text="Basic Backup - Configuration", 
                                       font=("Arial", 20, "bold"))
        self.title_label.pack(side="left", padx=20, pady=10)
        
        # Configuration content in scrollable frame
        self.scrollable_frame = ctk.CTkScrollableFrame(self)
        self.scrollable_frame.pack(fill="both", expand=True, padx=20, pady=20)
        
        # Appearance settings
        self.appearance_label = ctk.CTkLabel(self.scrollable_frame, 
                                           text="Appearance Settings:",
                                           font=("Arial", 16, "bold"))
        self.appearance_label.pack(anchor="w", pady=(0, 10))
        
        # Theme selection
        self.theme_label = ctk.CTkLabel(self.scrollable_frame, 
                                       text="Appearance Mode:", 
                                       font=("Arial", 14))
        self.theme_label.pack(anchor="w", pady=(10, 5))
        
        current_theme = config_manager.get('appearance.mode', "System")
        self.theme_var = ctk.StringVar(value=current_theme)
        self.theme_menu = ctk.CTkOptionMenu(self.scrollable_frame,
                                           values=["System", "Dark", "Light"],
                                           variable=self.theme_var,
                                           command=self.change_theme)
        self.theme_menu.pack(fill="x", pady=(0, 10))
        
        # Color theme selection
        self.color_theme_label = ctk.CTkLabel(self.scrollable_frame, 
                                             text="Color Theme:", 
                                             font=("Arial", 14))
        self.color_theme_label.pack(anchor="w", pady=(10, 5))
        
        current_color_theme = config_manager.get('appearance.theme', "blue")
        self.color_theme_var = ctk.StringVar(value=current_color_theme)
        self.color_theme_menu = ctk.CTkOptionMenu(self.scrollable_frame,
                                                 values=["blue", "green", "dark-blue"],
                                                 variable=self.color_theme_var,
                                                 command=self.change_color_theme)
        self.color_theme_menu.pack(fill="x", pady=(0, 20))
        
        # Media Settings
        self.media_label = ctk.CTkLabel(self.scrollable_frame, 
                                       text="Media Settings:",
                                       font=("Arial", 16, "bold"))
        self.media_label.pack(anchor="w", pady=(10, 10))
        
        # Add Media button and refresh button
        self.media_buttons_frame = ctk.CTkFrame(self.scrollable_frame, fg_color="transparent")
        self.media_buttons_frame.pack(fill="x", pady=(0, 10))
        
        self.media_btn = ctk.CTkButton(self.media_buttons_frame, 
                                      text="➕ Add Media",
                                      command=self.show_available_drives,
                                      height=40,
                                      font=("Arial", 14))
        self.media_btn.pack(side="left", padx=(0, 10))
        
        self.refresh_btn = ctk.CTkButton(self.media_buttons_frame, 
                                        text="🔄 Refresh",
                                        command=self.refresh_drives,
                                        height=40,
                                        font=("Arial", 14))
        self.refresh_btn.pack(side="left")
        
        # Media list frame
        self.media_frame = ctk.CTkFrame(self.scrollable_frame)
        self.media_frame.pack(fill="x", pady=(0, 20))
        
        self.media_list_label = ctk.CTkLabel(self.media_frame, 
                                            text="Available Drives:",
                                            font=("Arial", 14))
        self.media_list_label.pack(anchor="w", padx=10, pady=(10, 5))
        
        self.media_listbox = ctk.CTkTextbox(self.media_frame, 
                                           height=120,
                                           font=("Courier New", 11))  # Monospace for alignment
        self.media_listbox.pack(fill="x", padx=10, pady=(0, 10))
        self.media_listbox.configure(state="disabled")  # Make it read-only
        
        # Application settings
        self.settings_label = ctk.CTkLabel(self.scrollable_frame, 
                                          text="Application Settings:",
                                          font=("Arial", 16, "bold"))
        self.settings_label.pack(anchor="w", pady=(10, 10))
        
        current_auto_save = config_manager.get('application.auto_save', True)
        self.auto_save = ctk.CTkSwitch(self.scrollable_frame, 
                                      text="Enable Auto Save",
                                      command=self.on_auto_save_change)
        if current_auto_save:
            self.auto_save.select()
        self.auto_save.pack(anchor="w", pady=10)
        
        current_notifications = config_manager.get('application.notifications', True)
        self.notifications = ctk.CTkSwitch(self.scrollable_frame, 
                                          text="Enable Notifications",
                                          command=self.on_notifications_change)
        if current_notifications:
            self.notifications.select()
        self.notifications.pack(anchor="w", pady=10)
        
        # Configuration management
        self.management_label = ctk.CTkLabel(self.scrollable_frame, 
                                            text="Configuration Management:",
                                            font=("Arial", 16, "bold"))
        self.management_label.pack(anchor="w", pady=(20, 10))
        
        self.management_frame = ctk.CTkFrame(self.scrollable_frame)
        self.management_frame.pack(fill="x", pady=10)
        
        self.export_btn = ctk.CTkButton(self.management_frame, 
                                       text="Export Config",
                                       command=self.export_config)
        self.export_btn.pack(side="left", padx=(0, 10), pady=5)
        
        self.import_btn = ctk.CTkButton(self.management_frame, 
                                       text="Import Config",
                                       command=self.import_config)
        self.import_btn.pack(side="left", padx=(0, 10), pady=5)
        
        self.reset_btn = ctk.CTkButton(self.management_frame, 
                                      text="Reset to Defaults",
                                      command=self.reset_config,
                                      fg_color="orange",
                                      hover_color="dark orange")
        self.reset_btn.pack(side="left", pady=5)
        
        # Status label
        self.status_label = ctk.CTkLabel(self.scrollable_frame, 
                                        text="",
                                        text_color="green")
        self.status_label.pack(pady=10)
        
        # Load available drives on page show
        self.load_available_drives()
    
    def get_available_drives(self):
        """Get list of available drives on the system"""
        self.logger.info("Detecting available drives...")
        drives = []
        system = platform.system()
        
        try:
            if system == "Windows":
                self.logger.info("Windows system detected")
                # Windows: Use os module to get drive letters
                import string
                for drive_letter in string.ascii_uppercase:
                    drive_path = f"{drive_letter}:\\"
                    if os.path.exists(drive_path):
                        drives.append(drive_path)
                        self.logger.info(f"Found drive: {drive_path}")
                
            elif system == "Linux":
                self.logger.info("Linux system detected")
                # Linux: Multiple methods to find drives
                
                # Method 1: Check /proc/mounts
                if os.path.exists('/proc/mounts'):
                    with open('/proc/mounts', 'r') as f:
                        for line in f:
                            parts = line.split()
                            if len(parts) > 1:
                                mount_point = parts[1]
                                # Include common mount points
                                if any(mount_point.startswith(path) for path in ['/media/', '/mnt/', '/run/media/', '/home/']):
                                    if os.path.ismount(mount_point):
                                        drives.append(mount_point)
                                        self.logger.info(f"Found mount: {mount_point}")
                
                # Method 2: Use df command
                try:
                    result = subprocess.run(['df', '-h'], capture_output=True, text=True, timeout=5)
                    if result.returncode == 0:
                        lines = result.stdout.split('\n')[1:]  # Skip header
                        for line in lines:
                            if line.strip():
                                parts = line.split()
                                if len(parts) >= 6:
                                    mount_point = parts[5]
                                    if mount_point not in drives and mount_point != '/' and not mount_point.startswith('/boot'):
                                        drives.append(mount_point)
                                        self.logger.info(f"Found via df: {mount_point}")
                except (subprocess.TimeoutExpired, FileNotFoundError, subprocess.SubprocessError) as e:
                    self.logger.warning(f"df command failed: {e}")
                
                # Method 3: Check common directories
                for check_dir in ['/media', '/mnt', '/run/media']:
                    if os.path.exists(check_dir):
                        try:
                            for item in os.listdir(check_dir):
                                full_path = os.path.join(check_dir, item)
                                if os.path.ismount(full_path) and full_path not in drives:
                                    drives.append(full_path)
                                    self.logger.info(f"Found in {check_dir}: {full_path}")
                        except PermissionError:
                            self.logger.warning(f"Permission denied accessing {check_dir}")
                
                # Remove duplicates and sort
                drives = sorted(list(set(drives)))
                
            elif system == "Darwin":  # macOS
                self.logger.info("macOS system detected")
                # macOS: Check /Volumes for mounted drives
                volumes_dir = '/Volumes'
                if os.path.exists(volumes_dir):
                    try:
                        for item in os.listdir(volumes_dir):
                            full_path = os.path.join(volumes_dir, item)
                            if os.path.ismount(full_path):
                                drives.append(full_path)
                                self.logger.info(f"Found volume: {full_path}")
                    except PermissionError:
                        self.logger.warning("Permission denied accessing /Volumes")
            
            self.logger.info(f"Total drives found: {len(drives)}")
            
            # Get drive information for each drive
            drive_info = []
            for drive in drives:
                try:
                    if system == "Windows":
                        # Windows: Get drive type and free space
                        import ctypes
                        drive_type = ctypes.windll.kernel32.GetDriveTypeW(drive)
                        type_names = {
                            0: "Unknown",
                            1: "No Root",
                            2: "Removable",
                            3: "Fixed",
                            4: "Remote",
                            5: "CD-ROM",
                            6: "RAM Disk"
                        }
                        drive_type_name = type_names.get(drive_type, "Unknown")
                        
                        # Get free space
                        try:
                            free_bytes = ctypes.c_ulonglong(0)
                            total_bytes = ctypes.c_ulonglong(0)
                            free_bytes_ptr = ctypes.pointer(free_bytes)
                            total_bytes_ptr = ctypes.pointer(total_bytes)
                            
                            if ctypes.windll.kernel32.GetDiskFreeSpaceExW(
                                ctypes.c_wchar_p(drive), 
                                None, 
                                total_bytes_ptr, 
                                free_bytes_ptr
                            ):
                                free_gb = free_bytes.value / (1024**3)
                                total_gb = total_bytes.value / (1024**3)
                                usage_str = f" ({free_gb:.1f}GB free of {total_gb:.1f}GB)"
                            else:
                                usage_str = " (Size unknown)"
                                
                        except Exception as e:
                            self.logger.warning(f"Error getting size for {drive}: {e}")
                            usage_str = " (Size unknown)"
                        
                        drive_info.append(f"{drive} [{drive_type_name}]{usage_str}")
                    
                    else:
                        # Unix-like systems (Linux/macOS)
                        try:
                            stat = os.statvfs(drive)
                            free_gb = (stat.f_bavail * stat.f_frsize) / (1024**3)
                            total_gb = (stat.f_blocks * stat.f_frsize) / (1024**3)
                            usage_str = f" ({free_gb:.1f}GB free of {total_gb:.1f}GB)"
                        except Exception as e:
                            self.logger.warning(f"Error getting size for {drive}: {e}")
                            usage_str = " (Size unknown)"
                        
                        drive_info.append(f"{drive}{usage_str}")
                        
                except Exception as e:
                    error_msg = f"{drive} (Error: {str(e)})"
                    drive_info.append(error_msg)
                    self.logger.error(f"Error processing drive {drive}: {e}")
            
            return drive_info
            
        except Exception as e:
            error_msg = f"Error detecting drives: {str(e)}"
            self.logger.error(error_msg)
            return [error_msg]
    
    def load_available_drives(self):
        """Load and display available drives in the listbox"""
        self.logger.info("Loading available drives into UI")
        drives = self.get_available_drives()
        
        self.media_listbox.configure(state="normal")
        self.media_listbox.delete("1.0", "end")
        
        if drives:
            for i, drive in enumerate(drives, 1):
                self.media_listbox.insert("end", f"{i}. {drive}\n")
            self.logger.info(f"Displayed {len(drives)} drives")
        else:
            self.media_listbox.insert("end", "No drives found\n")
            self.logger.warning("No drives found to display")
        
        self.media_listbox.configure(state="disabled")
    
    def show_available_drives(self):
        """Show available drives with visual feedback"""
        self.logger.info("Add Media button clicked")
        self.media_btn.configure(state="disabled", text="🔄 Scanning...")
        self.update_idletasks()  # Force UI update
        
        try:
            self.load_available_drives()
            self.show_status("Drive list updated successfully")
            self.logger.info("Drive list updated successfully")
        except Exception as e:
            error_msg = f"Error updating drive list: {str(e)}"
            self.show_status(error_msg)
            self.logger.error(error_msg)
        finally:
            self.media_btn.configure(state="normal", text="➕ Add Media")
    
    def refresh_drives(self):
        """Alternative refresh method"""
        self.logger.info("Refresh button clicked")
        self.show_available_drives()
    
    def change_theme(self, choice):
        self.controller.update_appearance_mode(choice)
        self.show_status(f"Theme changed to {choice}")
    
    def change_color_theme(self, choice):
        self.controller.update_theme(choice)
        self.show_status(f"Color theme changed to {choice}")
    
    def on_auto_save_change(self):
        config_manager.set('application.auto_save', self.auto_save.get())
        self.show_status("Auto save setting updated")
    
    def on_notifications_change(self):
        config_manager.set('application.notifications', self.notifications.get())
        self.show_status("Notifications setting updated")
    
    def export_config(self):
        """Export configuration to file"""
        try:
            file_path = filedialog.asksaveasfilename(
                defaultextension=".yaml",
                filetypes=[("YAML files", "*.yaml"), ("All files", "*.*")],
                title="Export configuration to..."
            )
            
            if file_path:
                config_manager.export_config(file_path)
                self.show_status(f"Configuration exported to {os.path.basename(file_path)}")
                
        except Exception as e:
            messagebox.showerror("Export Error", f"Failed to export configuration: {e}")
    
    def import_config(self):
        """Import configuration from file"""
        try:
            file_path = filedialog.askopenfilename(
                filetypes=[("YAML files", "*.yaml"), ("All files", "*.*")],
                title="Import configuration from..."
            )
            
            if file_path:
                config_manager.import_config(file_path)
                self.load_current_config()  # Refresh UI with imported values
                
                # Update appearance
                self.controller.update_appearance_mode(config_manager.get('appearance.mode', 'System'))
                self.controller.update_theme(config_manager.get('appearance.theme', 'blue'))
                
                self.show_status("Configuration imported successfully!")
                
        except Exception as e:
            messagebox.showerror("Import Error", f"Failed to import configuration: {e}")
    
    def reset_config(self):
        """Reset configuration to defaults"""
        if messagebox.askyesno("Confirm Reset", "Are you sure you want to reset all settings to defaults?"):
            config_manager.reset_to_defaults()
            self.load_current_config()
            
            # Update appearance
            self.controller.update_appearance_mode(config_manager.get('appearance.mode', 'System'))
            self.controller.update_theme(config_manager.get('appearance.theme', 'blue'))
            
            self.show_status("Configuration reset to defaults")
    
    def load_current_config(self):
        """Load current configuration into UI"""
        # Update themes
        current_theme = config_manager.get('appearance.mode', "System")
        self.theme_var.set(current_theme)
        
        current_color_theme = config_manager.get('appearance.theme', "blue")
        self.color_theme_var.set(current_color_theme)
        
        # Update switches
        if config_manager.get('application.auto_save', True):
            self.auto_save.select()
        else:
            self.auto_save.deselect()
        
        if config_manager.get('application.notifications', True):
            self.notifications.select()
        else:
            self.notifications.deselect()
    
    def show_status(self, message: str):
        """Show status message"""
        self.status_label.configure(text=message)
        self.after(3000, lambda: self.status_label.configure(text=""))
    
    def on_page_show(self):
        """Called when page is shown - refresh configuration and drives"""
        self.load_current_config()
        self.load_available_drives()
        self.show_status("Configuration and drive list loaded")
