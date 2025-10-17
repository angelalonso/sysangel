import customtkinter as ctk
from .base_page import BasePage

class HomePage(BasePage):
    def setup_ui(self):
        # Configure grid weights to make the page expandable
        self.grid_rowconfigure(0, weight=1)
        self.grid_rowconfigure(1, weight=2)
        self.grid_rowconfigure(2, weight=1)
        self.grid_rowconfigure(3, weight=1)
        self.grid_columnconfigure(0, weight=1)
        
        # Main container that fills the entire page
        self.main_container = ctk.CTkFrame(self)
        self.main_container.grid(row=0, column=0, sticky="nsew", padx=20, pady=20)
        self.main_container.grid_rowconfigure(0, weight=1)
        self.main_container.grid_rowconfigure(1, weight=2)
        self.main_container.grid_rowconfigure(2, weight=1)
        self.main_container.grid_rowconfigure(3, weight=1)
        self.main_container.grid_columnconfigure(0, weight=1)
        
        # Header
        self.header_frame = ctk.CTkFrame(self.main_container, fg_color="transparent")
        self.header_frame.grid(row=0, column=0, sticky="nsew", pady=(20, 10))
        
        self.header = ctk.CTkLabel(self.header_frame, 
                                  text="Welcome to Basic Backup", 
                                  font=("Arial", 24, "bold"))
        self.header.pack(expand=True)
        
        # Information section - centered and expanded
        self.info_frame = ctk.CTkFrame(self.main_container)
        self.info_frame.grid(row=1, column=0, sticky="nsew", padx=50, pady=20)
        self.info_frame.grid_rowconfigure(0, weight=1)
        self.info_frame.grid_columnconfigure(0, weight=1)
        
        info_text = """Basic Backup - Your Simple Backup Solution

• Configuration: Set up backup media locations
• Backup: Create secure backups of your important data
• Restore: Easily restore your files from previous backups
• Settings: Customize appearance and backup defaults

Get started by selecting an option below:"""
        
        self.info_label = ctk.CTkLabel(self.info_frame, 
                                      text=info_text,
                                      font=("Arial", 14), 
                                      justify="center")
        self.info_label.grid(row=0, column=0, sticky="nsew", padx=30, pady=30)
        
        # First row of navigation buttons - main actions
        self.nav_frame_row1 = ctk.CTkFrame(self.main_container, fg_color="transparent")
        self.nav_frame_row1.grid(row=2, column=0, sticky="nsew", padx=80, pady=(10, 5))
        self.nav_frame_row1.grid_columnconfigure(0, weight=1)
        self.nav_frame_row1.grid_columnconfigure(1, weight=1)
        self.nav_frame_row1.grid_columnconfigure(2, weight=1)
        
        # Configure button
        self.config_btn = ctk.CTkButton(self.nav_frame_row1, 
                                       text="⚙️ Configure", 
                                       command=lambda: self.controller.show_page("ConfigurePage"),
                                       height=50,
                                       font=("Arial", 16))
        self.config_btn.grid(row=0, column=0, padx=8, pady=5, sticky="ew")
        
        # Backup button
        self.backup_btn = ctk.CTkButton(self.nav_frame_row1, 
                                       text="📦 Backup", 
                                       command=lambda: self.controller.show_page("BackupPage"),
                                       height=50,
                                       font=("Arial", 16))
        self.backup_btn.grid(row=0, column=1, padx=8, pady=5, sticky="ew")
        
        # Restore button
        self.restore_btn = ctk.CTkButton(self.nav_frame_row1, 
                                        text="🔄 Restore", 
                                        command=lambda: self.controller.show_page("RestorePage"),
                                        height=50,
                                        font=("Arial", 16))
        self.restore_btn.grid(row=0, column=2, padx=8, pady=5, sticky="ew")
        
        # Second row of navigation buttons - settings and exit
        self.nav_frame_row2 = ctk.CTkFrame(self.main_container, fg_color="transparent")
        self.nav_frame_row2.grid(row=3, column=0, sticky="nsew", padx=80, pady=(5, 20))
        self.nav_frame_row2.grid_columnconfigure(0, weight=1)
        self.nav_frame_row2.grid_columnconfigure(1, weight=1)
        
        # Settings button
        self.settings_btn = ctk.CTkButton(self.nav_frame_row2, 
                                         text="🔧 Settings", 
                                         command=lambda: self.controller.show_page("SettingsPage"),
                                         height=50,
                                         font=("Arial", 16),
                                         fg_color="#5bc0de",
                                         hover_color="#46b8da")
        self.settings_btn.grid(row=0, column=0, padx=8, pady=5, sticky="ew")
        
        # Exit button
        self.exit_btn = ctk.CTkButton(self.nav_frame_row2, 
                                     text="🚪 Exit", 
                                     command=self.exit_app,
                                     height=50,
                                     font=("Arial", 16),
                                     fg_color="#d9534f",
                                     hover_color="#c9302c")
        self.exit_btn.grid(row=0, column=1, padx=8, pady=5, sticky="ew")
    
    def exit_app(self):
        """Exit the application with confirmation"""
        from tkinter import messagebox
        if messagebox.askyesno("Exit", "Are you sure you want to exit Basic Backup?"):
            self.controller.quit()
            self.controller.destroy()
    
    def on_page_show(self):
        self.header.configure(text="Welcome to Basic Backup")
