import customtkinter as ctk
from cfg import ConfigManager
from gui import GTR2TalentsTunerApp

ctk.set_appearance_mode("System")
ctk.set_default_color_theme("blue")

def main():
    """Main entry point for the application"""
    # Load configuration first
    config_manager = ConfigManager()
    
    if not config_manager.load_configuration():
        return  # Configuration failed, app won't start
    
    # Create and run the application
    app = GTR2TalentsTunerApp(config_manager)
    app.protocol("WM_DELETE_WINDOW", app.on_closing)
    app.mainloop()

if __name__ == "__main__":
    main()
