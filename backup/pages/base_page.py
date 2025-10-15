import customtkinter as ctk

class BasePage(ctk.CTkFrame):
    """Base class for all pages with common functionality"""
    def __init__(self, parent, controller):
        super().__init__(parent)
        self.controller = controller
        self.setup_ui()
    
    def setup_ui(self):
        """Override this method in child classes to setup page UI"""
        pass
    
    def on_page_show(self):
        """Override this method to handle page display events"""
        pass
