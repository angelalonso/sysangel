import customtkinter as ctk
from tkinter import messagebox
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
                                       text="Backup", 
                                       font=("Arial", 20, "bold"))
        self.title_label.pack(side="left", padx=20, pady=10)
        
        # Backup content
        self.content_frame = ctk.CTkFrame(self)
        self.content_frame.pack(fill="both", expand=True, padx=20, pady=20)
        
        self.backup_info = ctk.CTkLabel(self.content_frame,
                                       text="Create a backup of your application data:",
                                       font=("Arial", 16))
        self.backup_info.pack(pady=(20, 10))
        
        # Backup options
        self.backup_type = ctk.CTkSegmentedButton(self.content_frame,
                                                 values=["Full Backup", "Incremental"])
        self.backup_type.set("Full Backup")
        self.backup_type.pack(pady=20)
        
        self.compression = ctk.CTkSwitch(self.content_frame,
                                        text="Enable Compression")
        self.compression.pack(pady=10)
        
        self.encryption = ctk.CTkSwitch(self.content_frame,
                                       text="Enable Encryption")
        self.encryption.pack(pady=10)
        
        # Backup button
        self.backup_btn = ctk.CTkButton(self.content_frame, 
                                       text="Start Backup",
                                       command=self.start_backup,
                                       height=40,
                                       font=("Arial", 16))
        self.backup_btn.pack(pady=30)
    
    def start_backup(self):
        # Simulate backup process
        messagebox.showinfo("Backup", "Backup process started successfully!")
