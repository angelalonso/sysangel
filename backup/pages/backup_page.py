import customtkinter as ctk
from tkinter import messagebox
from config.config_manager import config_manager
from .base_page import BasePage

class BackupPage(BasePage):
    def setup_ui(self):
        # Header with back button
        self.header_frame = ctk.CTkFrame(self)
        self.header_frame.pack(fill="x", padx=10, pady=10)
        
        self.back_btn = ctk.CTkButton(self.header_frame, 
                                     text="← Back", 
                                     command=lambda: self.controller.show_page("HomePage"),
                                     width=80)
        self.back_btn.pack(side="left", padx=10, pady=10)
        
        self.title_label = ctk.CTkLabel(self.header_frame, 
                                       text="Basic Backup - Backup", 
                                       font=("Arial", 20, "bold"))
        self.title_label.pack(side="left", padx=20, pady=10)
        
        # Backup content
        self.content_frame = ctk.CTkFrame(self)
        self.content_frame.pack(fill="both", expand=True, padx=20, pady=20)
        
        self.backup_info = ctk.CTkLabel(self.content_frame,
                                       text="Create a backup of your application data:",
                                       font=("Arial", 16))
        self.backup_info.pack(pady=(20, 10))
        
        # Backup type (now uses defaults from settings)
        backup_type_label = ctk.CTkLabel(self.content_frame,
                                        text="Backup Type:",
                                        font=("Arial", 14))
        backup_type_label.pack(anchor="w", pady=(10, 5))
        
        # Get default backup type from settings
        default_backup_type = config_manager.get('backup.default_type', 'full')
        self.backup_type = ctk.CTkSegmentedButton(self.content_frame,
                                                 values=["Full Backup", "Incremental"])
        self.backup_type.set("Full Backup" if default_backup_type == 'full' else "Incremental")
        self.backup_type.pack(fill="x", pady=(0, 20))
        
        # Backup options (now uses defaults from settings)
        options_label = ctk.CTkLabel(self.content_frame,
                                    text="Backup Options:",
                                    font=("Arial", 14))
        options_label.pack(anchor="w", pady=(10, 5))
        
        # Get default options from settings
        compression_enabled = config_manager.get('backup.compression', True)
        self.compression = ctk.CTkSwitch(self.content_frame,
                                        text="Enable Compression")
        if compression_enabled:
            self.compression.select()
        self.compression.pack(anchor="w", pady=10)
        
        encryption_enabled = config_manager.get('backup.encryption', False)
        self.encryption = ctk.CTkSwitch(self.content_frame,
                                       text="Enable Encryption")
        if encryption_enabled:
            self.encryption.select()
        self.encryption.pack(anchor="w", pady=10)
        
        # Backup button
        self.backup_btn = ctk.CTkButton(self.content_frame, 
                                       text="Start Backup",
                                       command=self.start_backup,
                                       height=40,
                                       font=("Arial", 16))
        self.backup_btn.pack(pady=30)
    
    def start_backup(self):
        """Start the actual backup process"""
        try:
            # Check if we have media and tiers configured
            configured_media = config_manager.get('backup.media', [])
            if not configured_media:
                messagebox.showwarning("No Media", "Please configure backup media first.")
                return
            
            # Get current settings
            backup_type = self.backup_type.get()
            compression = self.compression.get()
            encryption = self.encryption.get()
            
            # Show confirmation dialog
            confirm = messagebox.askyesno(
                "Start Backup", 
                f"Start {backup_type} backup?\n\n"
                f"Compression: {'Enabled' if compression else 'Disabled'}\n"
                f"Encryption: {'Enabled' if encryption else 'Disabled'}\n\n"
                "This may take a while depending on the amount of data."
            )
            
            if not confirm:
                return
            
            # Perform backup
            self._perform_backup(backup_type, compression, encryption)
            
        except Exception as e:
            messagebox.showerror("Backup Error", f"Failed to start backup: {e}")

    def _perform_backup(self, backup_type, compression, encryption):
        """Perform the backup operation"""
        try:
            from utils.backup_engine import BackupEngine
            
            backup_engine = BackupEngine()
            success = backup_engine.perform_backup(backup_type, compression, encryption)
            
            if success:
                messagebox.showinfo("Backup Complete", "Backup completed successfully!")
            else:
                messagebox.showerror("Backup Failed", "Backup failed. Check logs for details.")
                
        except Exception as e:
            messagebox.showerror("Backup Error", f"Backup failed: {e}")
    
    def on_page_show(self):
        """Called when page is shown"""
        pass
