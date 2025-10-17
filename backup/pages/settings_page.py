import customtkinter as ctk
from tkinter import messagebox
import logging
from config.config_manager import config_manager
from .base_page import BasePage
from utils.ui_utils import setup_scrollable_content, create_responsive_grid, create_section_header

class SettingsPage(BasePage):
    def setup_ui(self):
        # Set up logger for this class
        self.logger = logging.getLogger(__name__)
        
        # Configure the main frame to expand properly
        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, weight=1)
        
        # Header with back button
        self._create_header()
        
        # Main content with scrolling
        self.main_content, self.scrollable_frame = setup_scrollable_content(self)
        
        # Configure scrollable frame grid
        create_responsive_grid(self.scrollable_frame, rows=8, cols=3)
        
        # Create all sections
        self._create_appearance_section()
        self._create_backup_defaults_section()
        self._create_application_section()
        self._create_status_section()
    
    def _create_header(self):
        """Create the page header"""
        self.header_frame = ctk.CTkFrame(self)
        self.header_frame.pack(fill="x", padx=10, pady=10)
        
        self.back_btn = ctk.CTkButton(self.header_frame, 
                                     text="← Back", 
                                     command=lambda: self.controller.show_page("HomePage"),
                                     width=100)
        self.back_btn.pack(side="left", padx=10, pady=10)
        
        self.title_label = ctk.CTkLabel(self.header_frame, 
                                       text="Basic Backup - Settings", 
                                       font=("Arial", 20, "bold"))
        self.title_label.pack(side="left", padx=20, pady=10)
    
    def _create_appearance_section(self):
        """Create appearance settings section"""
        # Section header
        create_section_header(self.scrollable_frame, "Appearance Settings:", 0, columnspan=3)
        
        # Appearance mode
        ctk.CTkLabel(self.scrollable_frame, 
                    text="Appearance Mode:",
                    font=("Arial", 14)).grid(row=1, column=0, sticky="w", padx=10, pady=5)
        
        current_theme = config_manager.get('appearance.mode', "System")
        self.theme_var = ctk.StringVar(value=current_theme)
        self.theme_menu = ctk.CTkOptionMenu(self.scrollable_frame,
                                           values=["System", "Dark", "Light"],
                                           variable=self.theme_var,
                                           command=self.change_theme)
        self.theme_menu.grid(row=1, column=1, columnspan=2, sticky="ew", padx=10, pady=5)
        
        # Color theme
        ctk.CTkLabel(self.scrollable_frame, 
                    text="Color Theme:",
                    font=("Arial", 14)).grid(row=2, column=0, sticky="w", padx=10, pady=5)
        
        current_color_theme = config_manager.get('appearance.theme', "blue")
        self.color_theme_var = ctk.StringVar(value=current_color_theme)
        self.color_theme_menu = ctk.CTkOptionMenu(self.scrollable_frame,
                                                 values=["blue", "green", "dark-blue"],
                                                 variable=self.color_theme_var,
                                                 command=self.change_color_theme)
        self.color_theme_menu.grid(row=2, column=1, columnspan=2, sticky="ew", padx=10, pady=5)
    
    def _create_backup_defaults_section(self):
        """Create backup defaults section"""
        # Section header
        create_section_header(self.scrollable_frame, "Backup Defaults:", 3, columnspan=3)
        
        # Backup type
        ctk.CTkLabel(self.scrollable_frame, 
                    text="Default Backup Type:",
                    font=("Arial", 14)).grid(row=4, column=0, sticky="w", padx=10, pady=5)
        
        default_backup_type = config_manager.get('backup.default_type', 'full')
        self.backup_type_var = ctk.StringVar(value="Full Backup" if default_backup_type == 'full' else "Incremental")
        self.backup_type_menu = ctk.CTkOptionMenu(self.scrollable_frame,
                                                 values=["Full Backup", "Incremental"],
                                                 variable=self.backup_type_var,
                                                 command=self.on_backup_type_change)
        self.backup_type_menu.grid(row=4, column=1, columnspan=2, sticky="ew", padx=10, pady=5)
        
        # Compression default
        compression_enabled = config_manager.get('backup.compression', True)
        self.compression_switch = ctk.CTkSwitch(self.scrollable_frame, 
                                               text="Enable Compression by Default",
                                               command=self.on_compression_change)
        if compression_enabled:
            self.compression_switch.select()
        self.compression_switch.grid(row=5, column=0, columnspan=3, sticky="w", padx=10, pady=5)
        
        # Encryption default
        encryption_enabled = config_manager.get('backup.encryption', False)
        self.encryption_switch = ctk.CTkSwitch(self.scrollable_frame, 
                                              text="Enable Encryption by Default",
                                              command=self.on_encryption_change)
        if encryption_enabled:
            self.encryption_switch.select()
        self.encryption_switch.grid(row=6, column=0, columnspan=3, sticky="w", padx=10, pady=5)
    
    def _create_application_section(self):
        """Create application settings section"""
        # Section header
        create_section_header(self.scrollable_frame, "Application Settings:", 7, columnspan=3)
        
        # Auto save switch
        current_auto_save = config_manager.get('application.auto_save', True)
        self.auto_save = ctk.CTkSwitch(self.scrollable_frame, 
                                      text="Enable Auto Save",
                                      command=self.on_auto_save_change)
        if current_auto_save:
            self.auto_save.select()
        self.auto_save.grid(row=8, column=0, columnspan=3, sticky="w", padx=10, pady=5)
        
        # Notifications switch
        current_notifications = config_manager.get('application.notifications', True)
        self.notifications = ctk.CTkSwitch(self.scrollable_frame, 
                                          text="Enable Notifications",
                                          command=self.on_notifications_change)
        if current_notifications:
            self.notifications.select()
        self.notifications.grid(row=9, column=0, columnspan=3, sticky="w", padx=10, pady=5)
    
    def _create_status_section(self):
        """Create status section"""
        self.status_label = ctk.CTkLabel(self.scrollable_frame, 
                                        text="",
                                        text_color="green")
        self.status_label.grid(row=10, column=0, columnspan=3, sticky="w", padx=10, pady=10)
    
    def change_theme(self, choice):
        self.controller.update_appearance_mode(choice)
        self.show_status(f"Theme changed to {choice}")
    
    def change_color_theme(self, choice):
        self.controller.update_theme(choice)
        self.show_status(f"Color theme changed to {choice}")
    
    def on_backup_type_change(self, value):
        backup_type = 'full' if value == 'Full Backup' else 'incremental'
        config_manager.set('backup.default_type', backup_type)
        self.show_status(f"Default backup type set to {value}")
    
    def on_compression_change(self):
        config_manager.set('backup.compression', self.compression_switch.get())
        self.show_status("Compression default updated")
    
    def on_encryption_change(self):
        config_manager.set('backup.encryption', self.encryption_switch.get())
        self.show_status("Encryption default updated")
    
    def on_auto_save_change(self):
        config_manager.set('application.auto_save', self.auto_save.get())
        self.show_status("Auto save setting updated")
    
    def on_notifications_change(self):
        config_manager.set('application.notifications', self.notifications.get())
        self.show_status("Notifications setting updated")
    
    def show_status(self, message: str):
        """Show status message"""
        self.status_label.configure(text=message)
        self.after(3000, lambda: self.status_label.configure(text=""))
    
    def on_page_show(self):
        """Called when page is shown - refresh settings"""
        self.load_current_settings()
        self.show_status("Settings loaded")
    
    def load_current_settings(self):
        """Load current settings into UI"""
        # Update themes
        current_theme = config_manager.get('appearance.mode', "System")
        self.theme_var.set(current_theme)
        
        current_color_theme = config_manager.get('appearance.theme', "blue")
        self.color_theme_var.set(current_color_theme)
        
        # Update backup defaults
        default_backup_type = config_manager.get('backup.default_type', 'full')
        self.backup_type_var.set("Full Backup" if default_backup_type == 'full' else "Incremental")
        
        if config_manager.get('backup.compression', True):
            self.compression_switch.select()
        else:
            self.compression_switch.deselect()
        
        if config_manager.get('backup.encryption', False):
            self.encryption_switch.select()
        else:
            self.encryption_switch.deselect()
        
        # Update application settings
        if config_manager.get('application.auto_save', True):
            self.auto_save.select()
        else:
            self.auto_save.deselect()
        
        if config_manager.get('application.notifications', True):
            self.notifications.select()
        else:
            self.notifications.deselect()
