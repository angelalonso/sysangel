import customtkinter as ctk
from tkinter import messagebox
from .base_page import BasePage

class ConfigurePage(BasePage):
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
                                       text="Configuration", 
                                       font=("Arial", 20, "bold"))
        self.title_label.pack(side="left", padx=20, pady=10)
        
        # Configuration content
        self.content_frame = ctk.CTkFrame(self)
        self.content_frame.pack(fill="both", expand=True, padx=20, pady=20)
        
        # Example configuration options
        self.theme_label = ctk.CTkLabel(self.content_frame, 
                                       text="Appearance Mode:", 
                                       font=("Arial", 14))
        self.theme_label.pack(anchor="w", pady=(20, 5))
        
        self.theme_var = ctk.StringVar(value="System")
        self.theme_menu = ctk.CTkOptionMenu(self.content_frame,
                                           values=["System", "Dark", "Light"],
                                           variable=self.theme_var,
                                           command=self.change_theme)
        self.theme_menu.pack(fill="x", pady=(0, 20))
        
        self.auto_save = ctk.CTkSwitch(self.content_frame, 
                                      text="Enable Auto Save")
        self.auto_save.pack(anchor="w", pady=10)
        
        self.notifications = ctk.CTkSwitch(self.content_frame, 
                                          text="Enable Notifications")
        self.notifications.pack(anchor="w", pady=10)
        
        # Save button
        self.save_btn = ctk.CTkButton(self.content_frame, 
                                     text="Save Configuration",
                                     command=self.save_config)
        self.save_btn.pack(pady=20)
    
    def change_theme(self, choice):
        ctk.set_appearance_mode(choice)
    
    def save_config(self):
        messagebox.showinfo("Success", "Configuration saved successfully!")
