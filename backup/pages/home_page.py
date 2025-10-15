import customtkinter as ctk
from .base_page import BasePage

class HomePage(BasePage):
    def setup_ui(self):
        # Header
        self.header = ctk.CTkLabel(self, text="Welcome to Modern App", 
                                  font=("Arial", 24, "bold"))
        self.header.pack(pady=(20, 40))
        
        # Information section
        self.info_frame = ctk.CTkFrame(self)
        self.info_frame.pack(fill="x", padx=20, pady=10)
        
        info_text = """This is your modern Python application with multiple features:

• Configuration: Customize application settings
• Backup: Create backups of your data
• Restore: Restore from previous backups

Select an option below to get started."""
        
        self.info_label = ctk.CTkLabel(self.info_frame, text=info_text,
                                      font=("Arial", 14), justify="left")
        self.info_label.pack(padx=20, pady=20)
        
        # Navigation buttons
        self.nav_frame = ctk.CTkFrame(self)
        self.nav_frame.pack(fill="x", padx=50, pady=30)
        
        # Configure button
        self.config_btn = ctk.CTkButton(self.nav_frame, 
                                       text="⚙️ Configure", 
                                       command=lambda: self.controller.show_page("ConfigurePage"),
                                       height=40,
                                       font=("Arial", 16))
        self.config_btn.pack(fill="x", pady=10)
        
        # Backup button
        self.backup_btn = ctk.CTkButton(self.nav_frame, 
                                       text="📦 Backup", 
                                       command=lambda: self.controller.show_page("BackupPage"),
                                       height=40,
                                       font=("Arial", 16))
        self.backup_btn.pack(fill="x", pady=10)
        
        # Restore button
        self.restore_btn = ctk.CTkButton(self.nav_frame, 
                                        text="🔄 Restore", 
                                        command=lambda: self.controller.show_page("RestorePage"),
                                        height=40,
                                        font=("Arial", 16))
        self.restore_btn.pack(fill="x", pady=10)
    
    def on_page_show(self):
        self.header.configure(text="Welcome to Modern App")
