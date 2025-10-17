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
        
        # Configure the main frame to expand properly
        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, weight=1)
        
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
        
        # Main content container that fills available space
        self.main_content = ctk.CTkFrame(self)
        self.main_content.pack(fill="both", expand=True, padx=10, pady=10)
        self.main_content.grid_rowconfigure(0, weight=1)
        self.main_content.grid_columnconfigure(0, weight=1)
        
        # Configuration content in scrollable frame - properly configured to expand
        self.scrollable_frame = ctk.CTkScrollableFrame(self.main_content)
        self.scrollable_frame.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)
        
        # Configure scrollable frame to expand
        self.scrollable_frame.grid_rowconfigure(1, weight=1)
        self.scrollable_frame.grid_columnconfigure(0, weight=1)
        
        # Bind mouse wheel events to the scrollable frame and its children
        self._bind_mouse_wheel(self.scrollable_frame)
        
        # Create a content frame inside the scrollable frame for better layout control
        self.content_frame = ctk.CTkFrame(self.scrollable_frame)
        self.content_frame.pack(fill="both", expand=True, padx=10, pady=10)
        
        # Also bind mouse wheel to content frame for better coverage
        self._bind_mouse_wheel(self.content_frame)
        
        # Appearance settings
        self.appearance_label = ctk.CTkLabel(self.content_frame, 
                                           text="Appearance Settings:",
                                           font=("Arial", 16, "bold"))
        self.appearance_label.pack(anchor="w", pady=(0, 10))
        
        # Theme selection
        self.theme_label = ctk.CTkLabel(self.content_frame, 
                                       text="Appearance Mode:", 
                                       font=("Arial", 14))
        self.theme_label.pack(anchor="w", pady=(10, 5))
        
        current_theme = config_manager.get('appearance.mode', "System")
        self.theme_var = ctk.StringVar(value=current_theme)
        self.theme_menu = ctk.CTkOptionMenu(self.content_frame,
                                           values=["System", "Dark", "Light"],
                                           variable=self.theme_var,
                                           command=self.change_theme)
        self.theme_menu.pack(fill="x", pady=(0, 10))
        
        # Color theme selection
        self.color_theme_label = ctk.CTkLabel(self.content_frame, 
                                             text="Color Theme:", 
                                             font=("Arial", 14))
        self.color_theme_label.pack(anchor="w", pady=(10, 5))
        
        current_color_theme = config_manager.get('appearance.theme', "blue")
        self.color_theme_var = ctk.StringVar(value=current_color_theme)
        self.color_theme_menu = ctk.CTkOptionMenu(self.content_frame,
                                                 values=["blue", "green", "dark-blue"],
                                                 variable=self.color_theme_var,
                                                 command=self.change_color_theme)
        self.color_theme_menu.pack(fill="x", pady=(0, 20))
        
        # Media Settings
        self.media_label = ctk.CTkLabel(self.content_frame, 
                                       text="Backup Media:",
                                       font=("Arial", 16, "bold"))
        self.media_label.pack(anchor="w", pady=(10, 10))
        
        # Description
        self.media_desc = ctk.CTkLabel(self.content_frame, 
                                      text="Configure media locations where backups will be stored:",
                                      font=("Arial", 12),
                                      text_color="gray70")
        self.media_desc.pack(anchor="w", pady=(0, 10))
        
        # Add Media button
        self.media_btn = ctk.CTkButton(self.content_frame, 
                                      text="➕ Add Backup Media",
                                      command=self.show_add_media_dialog,
                                      height=40,
                                      font=("Arial", 14))
        self.media_btn.pack(fill="x", pady=(0, 15))
        
        # Configured Media list frame
        self.configured_media_frame = ctk.CTkFrame(self.content_frame)
        self.configured_media_frame.pack(fill="x", pady=(0, 20))
        
        self.configured_media_label = ctk.CTkLabel(self.configured_media_frame, 
                                                  text="Configured Backup Media:",
                                                  font=("Arial", 14, "bold"))
        self.configured_media_label.pack(anchor="w", padx=10, pady=(10, 5))
        
        # Container for configured media items
        self.configured_media_container = ctk.CTkFrame(self.configured_media_frame, fg_color="transparent")
        self.configured_media_container.pack(fill="x", padx=10, pady=(0, 10))
        
        # Application settings
        self.settings_label = ctk.CTkLabel(self.content_frame, 
                                          text="Application Settings:",
                                          font=("Arial", 16, "bold"))
        self.settings_label.pack(anchor="w", pady=(10, 10))
        
        current_auto_save = config_manager.get('application.auto_save', True)
        self.auto_save = ctk.CTkSwitch(self.content_frame, 
                                      text="Enable Auto Save",
                                      command=self.on_auto_save_change)
        if current_auto_save:
            self.auto_save.select()
        self.auto_save.pack(anchor="w", pady=10)
        
        current_notifications = config_manager.get('application.notifications', True)
        self.notifications = ctk.CTkSwitch(self.content_frame, 
                                          text="Enable Notifications",
                                          command=self.on_notifications_change)
        if current_notifications:
            self.notifications.select()
        self.notifications.pack(anchor="w", pady=10)
        
        # Configuration management
        self.management_label = ctk.CTkLabel(self.content_frame, 
                                            text="Configuration Management:",
                                            font=("Arial", 16, "bold"))
        self.management_label.pack(anchor="w", pady=(20, 10))
        
        self.management_frame = ctk.CTkFrame(self.content_frame)
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
        self.status_label = ctk.CTkLabel(self.content_frame, 
                                        text="",
                                        text_color="green")
        self.status_label.pack(pady=10)
        
        # Load configured media on page show
        self.load_configured_media()
    
    def _bind_mouse_wheel(self, widget):
        """Bind mouse wheel events to a widget for scrolling"""
        # Bind to the widget itself
        widget.bind("<MouseWheel>", self._on_mousewheel)
        widget.bind("<Button-4>", self._on_mousewheel)  # Linux scroll up
        widget.bind("<Button-5>", self._on_mousewheel)  # Linux scroll down
        
        # Also bind to all existing children
        for child in widget.winfo_children():
            child.bind("<MouseWheel>", self._on_mousewheel)
            child.bind("<Button-4>", self._on_mousewheel)
            child.bind("<Button-5>", self._on_mousewheel)

    def _on_mousewheel(self, event):
        """Handle mouse wheel events for scrolling"""
        # Get the scrollable frame that should handle the scrolling
        scrollable_frame = self.scrollable_frame
        
        if event.delta:
            # Windows and macOS
            scrollable_frame._parent_canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
        else:
            # Linux
            if event.num == 4:
                scrollable_frame._parent_canvas.yview_scroll(-1, "units")
            elif event.num == 5:
                scrollable_frame._parent_canvas.yview_scroll(1, "units")
    
    def get_available_drives(self):
        """Get list of available drives on the system for selection"""
        self.logger.info("Detecting available drives for media selection...")
        drives = []
        system = platform.system()
        
        try:
            if system == "Windows":
                self.logger.info("Windows system detected")
                import string
                for drive_letter in string.ascii_uppercase:
                    drive_path = f"{drive_letter}:\\"
                    if os.path.exists(drive_path):
                        drives.append(drive_path)
            
            elif system == "Linux":
                self.logger.info("Linux system detected")
                # System directories to exclude
                system_dirs = ['/dev', '/proc', '/sys', '/run', '/snap', '/boot', '/boot/efi']
                
                # Use df command to find mounted filesystems (most reliable)
                try:
                    result = subprocess.run(['df', '-h', '--output=target,size,avail'], 
                                          capture_output=True, text=True, timeout=5)
                    if result.returncode == 0:
                        lines = result.stdout.split('\n')[1:]  # Skip header
                        for line in lines:
                            if line.strip():
                                parts = line.split()
                                if len(parts) >= 3:
                                    mount_point = parts[0]
                                    size = parts[1]
                                    avail = parts[2]
                                    
                                    # Filter out system directories and root filesystem
                                    if (mount_point != '/' and 
                                        not any(mount_point.startswith(sys_dir) for sys_dir in system_dirs) and
                                        not mount_point.startswith('/var/lib/') and
                                        not mount_point.startswith('/tmp') and
                                        os.path.ismount(mount_point)):
                                        
                                        # Only include common user-accessible locations
                                        if (mount_point.startswith('/media/') or 
                                            mount_point.startswith('/mnt/') or
                                            mount_point.startswith('/run/media/') or
                                            mount_point.startswith('/home/')):
                                            drives.append(mount_point)
                                            self.logger.info(f"Found user media via df: {mount_point}")
                except (subprocess.TimeoutExpired, FileNotFoundError, subprocess.SubprocessError) as e:
                    self.logger.warning(f"df command failed: {e}")
                
                # Fallback: Check common media directories
                media_dirs = ['/media', '/mnt', '/run/media']
                for media_dir in media_dirs:
                    if os.path.exists(media_dir):
                        try:
                            for item in os.listdir(media_dir):
                                full_path = os.path.join(media_dir, item)
                                if (os.path.ismount(full_path) and 
                                    full_path not in drives and
                                    not any(full_path.startswith(sys_dir) for sys_dir in system_dirs)):
                                    drives.append(full_path)
                                    self.logger.info(f"Found media in {media_dir}: {full_path}")
                        except (PermissionError, OSError) as e:
                            self.logger.warning(f"Could not access {media_dir}: {e}")
                
                # Also check user's home directory for potential backup locations
                try:
                    home_dir = os.path.expanduser("~")
                    # Check if home is on a separate mount point (common in some setups)
                    if (os.path.ismount(home_dir) and 
                        home_dir not in drives and
                        home_dir != '/'):
                        drives.append(home_dir)
                        self.logger.info(f"Found home directory as separate mount: {home_dir}")
                except Exception as e:
                    self.logger.warning(f"Could not check home directory: {e}")
                
                # Remove duplicates and sort
                drives = sorted(list(set(drives)))
                
            elif system == "Darwin":  # macOS
                self.logger.info("macOS system detected")
                volumes_dir = '/Volumes'
                if os.path.exists(volumes_dir):
                    try:
                        for item in os.listdir(volumes_dir):
                            full_path = os.path.join(volumes_dir, item)
                            if os.path.ismount(full_path):
                                # Exclude system volumes on macOS
                                if not item.startswith('.') and item != 'Macintosh HD':
                                    drives.append(full_path)
                    except PermissionError:
                        self.logger.warning("Permission denied accessing /Volumes")
            
            self.logger.info(f"Total available drives found: {len(drives)}")
            return drives
            
        except Exception as e:
            self.logger.error(f"Error detecting drives: {e}")
            return []
    
    def show_add_media_dialog(self):
        """Show dialog to select and add backup media"""
        self.logger.info("Showing add media dialog")
        
        # Create dialog window
        dialog = ctk.CTkToplevel(self)
        dialog.title("Add Backup Media")
        dialog.geometry("500x350")
        dialog.resizable(True, True)
        dialog.minsize(450, 300)
        dialog.transient(self)
        
        # Configure dialog to expand properly
        dialog.grid_rowconfigure(0, weight=1)
        dialog.grid_columnconfigure(0, weight=1)
        
        # Main content frame that fills the dialog
        main_dialog_frame = ctk.CTkFrame(dialog)
        main_dialog_frame.grid(row=0, column=0, sticky="nsew", padx=10, pady=10)
        main_dialog_frame.grid_rowconfigure(1, weight=1)
        main_dialog_frame.grid_columnconfigure(0, weight=1)
        
        # Create a scrollable frame for the dialog content
        dialog_scrollable = ctk.CTkScrollableFrame(main_dialog_frame)
        dialog_scrollable.grid(row=0, column=0, sticky="nsew", pady=5)
        dialog_scrollable.grid_columnconfigure(0, weight=1)
        
        # Bind mouse wheel to dialog scrollable frame
        self._bind_dialog_mouse_wheel(dialog_scrollable, dialog)
        
        # Title section
        title_label = ctk.CTkLabel(dialog_scrollable, 
                                  text="Select Backup Media Location",
                                  font=("Arial", 16, "bold"))
        title_label.grid(row=0, column=0, sticky="w", pady=(0, 10))
        
        desc_label = ctk.CTkLabel(dialog_scrollable,
                                 text="Choose a location where backups will be stored:",
                                 font=("Arial", 12))
        desc_label.grid(row=1, column=0, sticky="w", pady=(0, 15))
        
        # Selection section
        selection_frame = ctk.CTkFrame(dialog_scrollable)
        selection_frame.grid(row=2, column=0, sticky="nsew", pady=10)
        selection_frame.grid_rowconfigure(2, weight=1)
        selection_frame.grid_columnconfigure(0, weight=1)
        
        selection_label = ctk.CTkLabel(selection_frame, 
                                      text="Available media:",
                                      font=("Arial", 12))
        selection_label.grid(row=0, column=0, sticky="w", pady=(10, 5))
        
        # Refresh button and dropdown in the same row
        refresh_select_frame = ctk.CTkFrame(selection_frame, fg_color="transparent")
        refresh_select_frame.grid(row=1, column=0, sticky="ew", pady=(0, 10))
        refresh_select_frame.grid_columnconfigure(0, weight=1)
        
        # Get available drives
        available_drives = self.get_available_drives()
        
        if not available_drives:
            available_drives = ["No media found - click Refresh"]
        
        selected_var = ctk.StringVar(value=available_drives[0] if available_drives else "")
        selection_menu = ctk.CTkOptionMenu(refresh_select_frame,
                                          values=available_drives,
                                          variable=selected_var)
        selection_menu.grid(row=0, column=0, sticky="ew", padx=(0, 10))
        
        def refresh_media_list():
            """Refresh the list of available media"""
            self.logger.info("Refreshing media list")
            new_drives = self.get_available_drives()
            
            if not new_drives:
                new_drives = ["No media found"]
            
            selection_menu.configure(values=new_drives)
            selected_var.set(new_drives[0] if new_drives else "")
            update_media_info()
        
        refresh_btn = ctk.CTkButton(refresh_select_frame,
                                   text="🔄",
                                   width=40,
                                   command=refresh_media_list)
        refresh_btn.grid(row=0, column=1, sticky="e")
        
        # Info label to show details about selected media
        info_label = ctk.CTkLabel(selection_frame,
                                 text="Select a media location to see details",
                                 font=("Arial", 10),
                                 text_color="gray70",
                                 wraplength=400,
                                 justify="left")
        info_label.grid(row=2, column=0, sticky="w", pady=(0, 10))
        
        def update_media_info(*args):
            """Update the info label with details about the selected media"""
            selected_path = selected_var.get()
            if selected_path and selected_path != "No media found" and selected_path != "No media found - click Refresh":
                try:
                    if platform.system() == "Windows":
                        import ctypes
                        drive_type = ctypes.windll.kernel32.GetDriveTypeW(selected_path)
                        type_names = {2: "Removable", 3: "Fixed", 4: "Remote", 5: "CD-ROM"}
                        drive_type_name = type_names.get(drive_type, "Unknown")
                        
                        free_bytes = ctypes.c_ulonglong(0)
                        total_bytes = ctypes.c_ulonglong(0)
                        if ctypes.windll.kernel32.GetDiskFreeSpaceExW(
                            ctypes.c_wchar_p(selected_path), 
                            None, 
                            ctypes.pointer(total_bytes), 
                            ctypes.pointer(free_bytes)
                        ):
                            free_gb = free_bytes.value / (1024**3)
                            total_gb = total_bytes.value / (1024**3)
                            info_label.configure(text=f"Type: {drive_type_name} | Free: {free_gb:.1f}GB / Total: {total_gb:.1f}GB")
                        else:
                            info_label.configure(text=f"Type: {drive_type_name}")
                    else:
                        stat = os.statvfs(selected_path)
                        free_gb = (stat.f_bavail * stat.f_frsize) / (1024**3)
                        total_gb = (stat.f_blocks * stat.f_frsize) / (1024**3)
                        used_percent = ((total_gb - free_gb) / total_gb) * 100
                        
                        media_type = "Storage"
                        if '/media/' in selected_path or '/run/media/' in selected_path:
                            media_type = "Removable Media"
                        elif '/mnt/' in selected_path:
                            media_type = "Mounted Storage"
                        elif selected_path.startswith('/home/'):
                            media_type = "Home Directory"
                        
                        info_label.configure(text=f"Type: {media_type} | Free: {free_gb:.1f}GB / Total: {total_gb:.1f}GB ({used_percent:.1f}% used)")
                except Exception as e:
                    info_label.configure(text=f"Could not get media information: {str(e)}")
            else:
                info_label.configure(text="Select a media location to see details")
        
        selected_var.trace('w', update_media_info)
        update_media_info()
        
        # Buttons frame at the bottom (outside scrollable area)
        buttons_frame = ctk.CTkFrame(main_dialog_frame, fg_color="transparent")
        buttons_frame.grid(row=1, column=0, sticky="ew", pady=15)
        buttons_frame.grid_columnconfigure(0, weight=1)
        
        def add_selected_media():
            selected_path = selected_var.get()
            if selected_path and selected_path != "No media found" and selected_path != "No media found - click Refresh":
                self.add_media_to_config(selected_path)
                dialog.destroy()
            else:
                messagebox.showwarning("Invalid Selection", "Please select a valid media location.")
        
        add_btn = ctk.CTkButton(buttons_frame, 
                               text="Add Selected Media",
                               command=add_selected_media,
                               height=35)
        add_btn.pack(side="right", padx=(10, 0))
        
        cancel_btn = ctk.CTkButton(buttons_frame, 
                                  text="Cancel",
                                  command=dialog.destroy,
                                  height=35,
                                  fg_color="gray")
        cancel_btn.pack(side="right")
        
        # Center the dialog
        dialog.update_idletasks()
        x = self.winfo_x() + (self.winfo_width() - dialog.winfo_width()) // 2
        y = self.winfo_y() + (self.winfo_height() - dialog.winfo_height()) // 2
        dialog.geometry(f"+{x}+{y}")
        
        # Make dialog modal
        dialog.focus_set()
        dialog.grab_set()
        
        # Wait for the window to be closed
        self.wait_window(dialog)

    def _bind_dialog_mouse_wheel(self, widget, dialog):
        """Bind mouse wheel events specifically for dialog scrolling"""
        def on_dialog_mousewheel(event):
            if event.delta:
                # Windows and macOS
                widget._parent_canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
            else:
                # Linux
                if event.num == 4:
                    widget._parent_canvas.yview_scroll(-1, "units")
                elif event.num == 5:
                    widget._parent_canvas.yview_scroll(1, "units")
        
        # Bind to the widget and its children
        widget.bind("<MouseWheel>", on_dialog_mousewheel)
        widget.bind("<Button-4>", on_dialog_mousewheel)
        widget.bind("<Button-5>", on_dialog_mousewheel)
        
        for child in widget.winfo_children():
            child.bind("<MouseWheel>", on_dialog_mousewheel)
            child.bind("<Button-4>", on_dialog_mousewheel)
            child.bind("<Button-5>", on_dialog_mousewheel)
    
    def add_media_to_config(self, media_path):
        """Add media path to configuration"""
        try:
            # Get current configured media
            configured_media = config_manager.get('backup.media', [])
            
            # Check if already exists
            if media_path in configured_media:
                self.show_status(f"Media already configured: {media_path}")
                return
            
            # Add to configured media
            configured_media.append(media_path)
            config_manager.set('backup.media', configured_media)
            
            # Refresh the display
            self.load_configured_media()
            
            self.show_status(f"Media added: {media_path}")
            self.logger.info(f"Added media: {media_path}")
            
        except Exception as e:
            error_msg = f"Error adding media: {str(e)}"
            self.show_status(error_msg)
            self.logger.error(error_msg)
    
    def remove_media_from_config(self, media_path):
        """Remove media path from configuration"""
        try:
            # Get current configured media
            configured_media = config_manager.get('backup.media', [])
            
            # Remove the media
            if media_path in configured_media:
                configured_media.remove(media_path)
                config_manager.set('backup.media', configured_media)
                
                # Refresh the display
                self.load_configured_media()
                
                self.show_status(f"Media removed: {media_path}")
                self.logger.info(f"Removed media: {media_path}")
            else:
                self.show_status("Media not found in configuration")
                
        except Exception as e:
            error_msg = f"Error removing media: {str(e)}"
            self.show_status(error_msg)
            self.logger.error(error_msg)
    
    def load_configured_media(self):
        """Load and display configured media"""
        self.logger.info("Loading configured media")
        
        # Clear existing media widgets
        for widget in self.configured_media_container.winfo_children():
            widget.destroy()
        
        # Get configured media from config
        configured_media = config_manager.get('backup.media', [])
        
        if not configured_media:
            # Show message when no media configured
            no_media_label = ctk.CTkLabel(self.configured_media_container,
                                         text="No backup media configured. Click 'Add Backup Media' to get started.",
                                         font=("Arial", 12),
                                         text_color="gray60")
            no_media_label.pack(pady=20)
            return
        
        # Display each configured media item
        for i, media_path in enumerate(configured_media):
            media_item_frame = ctk.CTkFrame(self.configured_media_container)
            media_item_frame.pack(fill="x", pady=5)
            
            # Media path label
            path_label = ctk.CTkLabel(media_item_frame,
                                     text=media_path,
                                     font=("Courier New", 11),
                                     anchor="w")
            path_label.pack(side="left", padx=10, pady=5, fill="x", expand=True)
            
            # Remove button
            remove_btn = ctk.CTkButton(media_item_frame,
                                      text="✕",
                                      width=30,
                                      height=30,
                                      fg_color="#d9534f",
                                      hover_color="#c9302c",
                                      command=lambda path=media_path: self.remove_media_from_config(path))
            remove_btn.pack(side="right", padx=5, pady=5)
        
        self.logger.info(f"Displayed {len(configured_media)} configured media items")
    
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
        
        # Load configured media
        self.load_configured_media()
    
    def show_status(self, message: str):
        """Show status message"""
        self.status_label.configure(text=message)
        self.after(3000, lambda: self.status_label.configure(text=""))
    
    def on_page_show(self):
        """Called when page is shown - refresh configuration and media"""
        self.load_current_config()
        self.load_configured_media()
        self.show_status("Configuration and media list loaded")
