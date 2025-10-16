import customtkinter as ctk
from config.config_manager import config_manager
from pages.home_page import HomePage
from pages.configure_page import ConfigurePage
from pages.backup_page import BackupPage
from pages.restore_page import RestorePage

class MainApp(ctk.CTk):
    def __init__(self):
        super().__init__()
        
        # Load appearance mode from config
        appearance_mode = config_manager.get('appearance.mode', "System")
        theme = config_manager.get('appearance.theme', "blue")
        
        ctk.set_appearance_mode(appearance_mode)
        ctk.set_default_color_theme(theme)
        
        self.title("Basic Backup")
        self.geometry("800x600")
        self.minsize(700, 500)
        
        # Container for all pages
        self.container = ctk.CTkFrame(self)
        self.container.pack(fill="both", expand=True, padx=10, pady=10)
        
        # Dictionary to hold all pages
        self.pages = {}
        
        # Initialize all pages
        for PageClass in (HomePage, ConfigurePage, BackupPage, RestorePage):
            page_name = PageClass.__name__
            page = PageClass(parent=self.container, controller=self)
            self.pages[page_name] = page
            page.grid(row=0, column=0, sticky="nsew")
        
        # Show home page initially
        self.show_page("HomePage")
    
    def show_page(self, page_name):
        """Show the specified page and hide others"""
        page = self.pages[page_name]
        page.tkraise()
        page.on_page_show()
    
    def update_appearance_mode(self, mode: str):
        """Update appearance mode and save to config"""
        ctk.set_appearance_mode(mode)
        config_manager.set('appearance.mode', mode)
    
    def update_theme(self, theme: str):
        """Update color theme and save to config"""
        ctk.set_default_color_theme(theme)
        config_manager.set('appearance.theme', theme)
