import customtkinter as ctk
from tkinter import messagebox
from .base_page import BasePage

class RestorePage(BasePage):
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
                                       text="Basic Backup - Restore", 
                                       font=("Arial", 20, "bold"))
        self.title_label.pack(side="left", padx=20, pady=10)
        
        # Restore content
        self.content_frame = ctk.CTkFrame(self)
        self.content_frame.pack(fill="both", expand=True, padx=20, pady=20)
        
        self.restore_info = ctk.CTkLabel(self.content_frame,
                                        text="Restore your application from a backup:",
                                        font=("Arial", 16))
        self.restore_info.pack(pady=(20, 10))
        
        # Backup selection
        self.backup_list_label = ctk.CTkLabel(self.content_frame,
                                             text="Available Backups:",
                                             font=("Arial", 14))
        self.backup_list_label.pack(anchor="w", pady=(20, 5))
        
        # Simulated backup list
        backups = ["Backup_2024_01_15", "Backup_2024_01_10", "Backup_2024_01_05"]
        self.backup_var = ctk.StringVar(value=backups[0])
        self.backup_menu = ctk.CTkOptionMenu(self.content_frame,
                                            values=backups,
                                            variable=self.backup_var)
        self.backup_menu.pack(fill="x", pady=(0, 20))
        
        # Restore options
        self.verify_backup = ctk.CTkSwitch(self.content_frame,
                                          text="Verify backup before restore")
        self.verify_backup.pack(anchor="w", pady=10)
        
        # Restore button
        self.restore_btn = ctk.CTkButton(self.content_frame, 
                                        text="Restore Selected Backup",
                                        command=self.start_restore,
                                        height=40,
                                        font=("Arial", 16))
        self.restore_btn.pack(pady=30)
    
    def start_restore(self):
        selected_backup = self.backup_var.get()
        messagebox.showinfo("Restore", f"Restoring from: {selected_backup}")
    
    def on_page_show(self):
        """Called when page is shown"""
        pass
