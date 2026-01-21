import customtkinter as ctk
import tkinter as tk
from tkinter import messagebox

ctk.set_appearance_mode("System")  # Modes: "System", "Dark", "Light"
ctk.set_default_color_theme("blue")  # Themes: "blue", "green", "dark-blue"

class App(ctk.CTk):
    def __init__(self):
        super().__init__()
        
        self.title("GTR2 Talents tuner")
        self.geometry("800x600")
        
        # Create modern UI elements
        self.label = ctk.CTkLabel(self, text="Welcome to Modern App", 
                                 font=("Arial", 20))
        self.label.pack(pady=20)
        
        self.entry = ctk.CTkEntry(self, placeholder_text="Enter something...")
        self.entry.pack(pady=10)
        
        self.button = ctk.CTkButton(self, text="Click Me", command=self.button_click)
        self.button.pack(pady=10)
        
        self.switch = ctk.CTkSwitch(self, text="Dark Mode")
        self.switch.pack(pady=10)
    
    def button_click(self):
        messagebox.showinfo("Info", f"You entered: {self.entry.get()}")

if __name__ == "__main__":
    app = App()
    app.mainloop()
