import customtkinter as ctk
from tkinter import messagebox
from utils.file_utils import get_available_drives, get_media_info
from config.config_manager import config_manager
import logging

class MediaDialog:
    """Handles the media selection dialog functionality"""
    
    def __init__(self, parent, on_media_selected: callable):
        self.parent = parent
        self.on_media_selected = on_media_selected
        self.logger = logging.getLogger(__name__)
    
    def show(self):
        """Show the media selection dialog"""
        from utils.ui_utils import create_modal_dialog
        
        # Create a more compact dialog
        self.dialog = create_modal_dialog(self.parent, "Add Backup Media", 550, 300)
        
        # Configure dialog grid
        self.dialog.grid_rowconfigure(0, weight=0)  # Title
        self.dialog.grid_rowconfigure(1, weight=0)  # Description
        self.dialog.grid_rowconfigure(2, weight=0)  # Selection
        self.dialog.grid_rowconfigure(3, weight=1)  # Info (can expand)
        self.dialog.grid_rowconfigure(4, weight=0)  # Buttons
        self.dialog.grid_columnconfigure(0, weight=1)
        
        # Dialog content using grid for better control
        self._create_dialog_content()
        
        # Wait for the window to be closed
        self.parent.wait_window(self.dialog)
    
    def _create_dialog_content(self):
        """Create the dialog content using grid layout"""
        # Title
        title_label = ctk.CTkLabel(self.dialog, 
                                  text="Select Backup Media Location",
                                  font=("Arial", 16, "bold"))
        title_label.grid(row=0, column=0, sticky="w", padx=20, pady=(15, 5))
        
        # Description
        desc_label = ctk.CTkLabel(self.dialog,
                                 text="Choose a location where backups will be stored:",
                                 font=("Arial", 12))
        desc_label.grid(row=1, column=0, sticky="w", padx=20, pady=(0, 15))
        
        # Selection area
        selection_frame = ctk.CTkFrame(self.dialog)
        selection_frame.grid(row=2, column=0, sticky="ew", padx=20, pady=10)
        selection_frame.grid_columnconfigure(0, weight=1)
        
        # Selection label
        selection_label = ctk.CTkLabel(selection_frame, 
                                      text="Available media:",
                                      font=("Arial", 12))
        selection_label.grid(row=0, column=0, sticky="w", padx=10, pady=(10, 5))
        
        # Dropdown and refresh button
        dropdown_frame = ctk.CTkFrame(selection_frame, fg_color="transparent")
        dropdown_frame.grid(row=1, column=0, sticky="ew", padx=10, pady=(0, 10))
        dropdown_frame.grid_columnconfigure(0, weight=1)
        
        # Get available drives
        available_drives = get_available_drives()
        
        if not available_drives:
            available_drives = ["No media found - click Refresh"]
        
        self.selected_var = ctk.StringVar(value=available_drives[0] if available_drives else "")
        self.selection_menu = ctk.CTkOptionMenu(dropdown_frame,
                                              values=available_drives,
                                              variable=self.selected_var)
        self.selection_menu.grid(row=0, column=0, sticky="ew", padx=(0, 10))
        
        refresh_btn = ctk.CTkButton(dropdown_frame,
                                   text="🔄",
                                   width=40,
                                   command=self._refresh_media_list)
        refresh_btn.grid(row=0, column=1, sticky="e")
        
        # Info label
        self.info_label = ctk.CTkLabel(self.dialog,
                                     text="Select a media location to see details",
                                     font=("Arial", 10),
                                     text_color="gray70",
                                     wraplength=500,
                                     justify="left")
        self.info_label.grid(row=3, column=0, sticky="w", padx=20, pady=(0, 10))
        
        # Update info when selection changes
        self.selected_var.trace('w', self._update_media_info)
        self._update_media_info()
        
        # Buttons frame
        buttons_frame = ctk.CTkFrame(self.dialog, fg_color="transparent")
        buttons_frame.grid(row=4, column=0, sticky="e", padx=20, pady=15)
        
        add_btn = ctk.CTkButton(buttons_frame, 
                               text="Add Selected Media",
                               command=self._add_selected_media,
                               height=35)
        add_btn.pack(side="right", padx=(10, 0))
        
        cancel_btn = ctk.CTkButton(buttons_frame, 
                                  text="Cancel",
                                  command=self.dialog.destroy,
                                  height=35,
                                  fg_color="gray")
        cancel_btn.pack(side="right")
    
    def _refresh_media_list(self):
        """Refresh the list of available media"""
        self.logger.info("Refreshing media list")
        new_drives = get_available_drives()
        
        if not new_drives:
            new_drives = ["No media found"]
        
        self.selection_menu.configure(values=new_drives)
        self.selected_var.set(new_drives[0] if new_drives else "")
        self._update_media_info()
    
    def _update_media_info(self, *args):
        """Update the info label with details about the selected media"""
        selected_path = self.selected_var.get()
        if selected_path and selected_path != "No media found" and selected_path != "No media found - click Refresh":
            try:
                info = get_media_info(selected_path)
                if info['error']:
                    self.info_label.configure(text=f"Could not get media information: {info['error']}")
                else:
                    used_percent = ((info['total_gb'] - info['free_gb']) / info['total_gb']) * 100 if info['total_gb'] > 0 else 0
                    self.info_label.configure(
                        text=f"Type: {info['type']} | Free: {info['free_gb']:.1f}GB / Total: {info['total_gb']:.1f}GB ({used_percent:.1f}% used)"
                    )
            except Exception as e:
                self.info_label.configure(text=f"Could not get media information: {str(e)}")
        else:
            self.info_label.configure(text="Select a media location to see details")
    
    def _add_selected_media(self):
        """Add the selected media to configuration"""
        selected_path = self.selected_var.get()
        if selected_path and selected_path != "No media found" and selected_path != "No media found - click Refresh":
            self.on_media_selected(selected_path)
            self.dialog.destroy()
        else:
            messagebox.showwarning("Invalid Selection", "Please select a valid media location.")
