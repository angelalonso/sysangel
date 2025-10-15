import customtkinter as ctk
from pages.home_page import HomePage
from pages.configure_page import ConfigurePage
from pages.backup_page import BackupPage
from pages.restore_page import RestorePage

ctk.set_appearance_mode("System")
ctk.set_default_color_theme("blue")

class MainApp(ctk.CTk):
    def __init__(self):
        super().__init__()
        
        self.title("Simple Backup")
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
        page.on_page_show()  # Notify page that it's being shown
