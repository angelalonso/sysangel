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
        
        # Backup type
        backup_type_label = ctk.CTkLabel(self.content_frame,
                                        text="Backup Type:",
                                        font=("Arial", 14))
        backup_type_label.pack(anchor="w", pady=(10, 5))
        
        default_backup_type = config_manager.get('backup.default_type', 'full')
        self.backup_type = ctk.CTkSegmentedButton(self.content_frame,
                                                 values=["Full Backup", "Incremental"],
                                                 command=self.on_backup_type_change)
        self.backup_type.set("Full Backup" if default_backup_type == 'full' else "Incremental")
        self.backup_type.pack(fill="x", pady=(0, 20))
        
        # Backup options
        options_label = ctk.CTkLabel(self.content_frame,
                                    text="Backup Options:",
                                    font=("Arial", 14))
        options_label.pack(anchor="w", pady=(10, 5))
        
        compression_enabled = config_manager.get('backup.compression', True)
        self.compression = ctk.CTkSwitch(self.content_frame,
                                        text="Enable Compression",
                                        command=self.on_compression_change)
        if compression_enabled:
            self.compression.select()
        self.compression.pack(anchor="w", pady=10)
        
        encryption_enabled = config_manager.get('backup.encryption', False)
        self.encryption = ctk.CTkSwitch(self.content_frame,
                                       text="Enable Encryption",
                                       command=self.on_encryption_change)
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
    
    def on_backup_type_change(self, value):
        backup_type = 'full' if value == 'Full Backup' else 'incremental'
        config_manager.set('backup.default_type', backup_type)
    
    def on_compression_change(self):
        config_manager.set('backup.compression', self.compression.get())
    
    def on_encryption_change(self):
        config_manager.set('backup.encryption', self.encryption.get())
    
    def start_backup(self):
        # Get current settings
        backup_type = self.backup_type.get()
        compression = self.compression.get()
        encryption = self.encryption.get()
        
        # Simulate backup process with actual settings
        message = f"Backup started!\nType: {backup_type}\nCompression: {compression}\nEncryption: {encryption}"
        messagebox.showinfo("Backup", message)
