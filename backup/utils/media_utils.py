import customtkinter as ctk
from tkinter import messagebox
from utils.file_utils import get_available_drives, get_media_info
from config.config_manager import config_manager
import logging

class MediaDialogWithTiers:
    """Handles the media selection dialog with tier assignment"""
    
    def __init__(self, parent, on_media_selected: callable, media_path: str = None, selected_tiers: list = None):
        self.parent = parent
        self.on_media_selected = on_media_selected
        self.media_path = media_path  # For editing existing media
        self.selected_tiers = selected_tiers or []  # For editing existing media
        self.logger = logging.getLogger(__name__)
    
    def show(self):
        """Show the media selection dialog with tier assignment"""
        from utils.ui_utils import create_modal_dialog
        
        # Create a larger dialog to accommodate tier selection
        title = "Edit Backup Media" if self.media_path else "Add Backup Media"
        self.dialog = create_modal_dialog(self.parent, title, 600, 500)
        
        # Configure dialog grid
        self.dialog.grid_rowconfigure(2, weight=1)
        self.dialog.grid_columnconfigure(0, weight=1)
        
        # Dialog content
        self._create_dialog_content()
        
        # Wait for the window to be closed
        self.parent.wait_window(self.dialog)
    
    def _create_dialog_content(self):
        """Create the dialog content with tier selection"""
        # Title
        title_text = "Edit Backup Media" if self.media_path else "Select Backup Media Location"
        title_label = ctk.CTkLabel(self.dialog, 
                                  text=title_text,
                                  font=("Arial", 16, "bold"))
        title_label.grid(row=0, column=0, sticky="w", padx=20, pady=(15, 10))
        
        # Media selection section (only for new media)
        if not self.media_path:
            desc_label = ctk.CTkLabel(self.dialog,
                                     text="Choose a location where backups will be stored:",
                                     font=("Arial", 12))
            desc_label.grid(row=1, column=0, sticky="w", padx=20, pady=(0, 10))
            
            # Selection area
            selection_frame = ctk.CTkFrame(self.dialog)
            selection_frame.grid(row=2, column=0, sticky="ew", padx=20, pady=5)
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
            self.info_label = ctk.CTkLabel(selection_frame,
                                         text="Select a media location to see details",
                                         font=("Arial", 10),
                                         text_color="gray70",
                                         wraplength=500,
                                         justify="left")
            self.info_label.grid(row=2, column=0, sticky="w", padx=10, pady=(0, 10))
            
            # Update info when selection changes
            self.selected_var.trace('w', self._update_media_info)
            self._update_media_info()
            
            row_offset = 3
        else:
            # For editing existing media, show the path
            path_frame = ctk.CTkFrame(self.dialog)
            path_frame.grid(row=1, column=0, sticky="ew", padx=20, pady=5)
            
            ctk.CTkLabel(path_frame,
                        text="Media Path:",
                        font=("Arial", 12)).grid(row=0, column=0, sticky="w", padx=10, pady=5)
            
            ctk.CTkLabel(path_frame,
                        text=self.media_path,
                        font=("Courier New", 11)).grid(row=0, column=1, sticky="w", padx=10, pady=5)
            
            row_offset = 2
        
        # Tier selection section
        tier_frame = ctk.CTkFrame(self.dialog)
        tier_frame.grid(row=row_offset, column=0, sticky="nsew", padx=20, pady=10)
        tier_frame.grid_rowconfigure(1, weight=1)
        tier_frame.grid_columnconfigure(0, weight=1)
        
        tier_label = ctk.CTkLabel(tier_frame, 
                                 text="Assign Backup Tiers:",
                                 font=("Arial", 14, "bold"))
        tier_label.grid(row=0, column=0, sticky="w", padx=10, pady=(10, 5))
        
        # Get tier configuration for display
        tiers_config = config_manager.get('backup.tiers', self._get_default_tiers())
        
        # Tier selection checkboxes
        self.tier_vars = {}
        tier_checkboxes_frame = ctk.CTkFrame(tier_frame, fg_color="transparent")
        tier_checkboxes_frame.grid(row=1, column=0, sticky="w", padx=10, pady=5)
        
        for i, (tier_id, tier_config) in enumerate(tiers_config.items()):
            var = ctk.BooleanVar(value=tier_id in self.selected_tiers)
            self.tier_vars[tier_id] = var
            
            checkbox = ctk.CTkCheckBox(tier_checkboxes_frame,
                                     text=f"{tier_config['name']}: {tier_config['description']}",
                                     variable=var,
                                     font=("Arial", 12))
            checkbox.grid(row=i, column=0, sticky="w", pady=2)
        
        # Buttons frame
        buttons_frame = ctk.CTkFrame(self.dialog, fg_color="transparent")
        buttons_frame.grid(row=row_offset + 1, column=0, sticky="e", padx=20, pady=15)
        
        save_text = "Update Media" if self.media_path else "Add Selected Media"
        save_btn = ctk.CTkButton(buttons_frame, 
                                text=save_text,
                                command=self._save_media,
                                height=35)
        save_btn.pack(side="right", padx=(10, 0))
        
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
        if hasattr(self, 'selected_var'):
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
    
    def _save_media(self):
        """Save the media configuration with tier assignments"""
        try:
            # Get selected tiers
            selected_tiers = [tier_id for tier_id, var in self.tier_vars.items() if var.get()]
            
            if not selected_tiers:
                messagebox.showwarning("No Tiers Selected", "Please select at least one backup tier")
                return
            
            # Get media path
            if self.media_path:
                media_path = self.media_path  # Use existing path for editing
            else:
                media_path = self.selected_var.get()
                if not media_path or media_path in ["No media found", "No media found - click Refresh"]:
                    messagebox.showwarning("Invalid Selection", "Please select a valid media location")
                    return
            
            # Call the callback
            self.on_media_selected(media_path, selected_tiers)
            self.dialog.destroy()
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save media configuration: {str(e)}")
    
    def _get_default_tiers(self):
        """Return default tier configuration"""
        return {
            'tier1': {
                'name': 'Critical Data',
                'description': 'Essential system files and critical user data',
                'include': [],
                'exclude': []
            },
            'tier2': {
                'name': 'Important Data', 
                'description': 'Important documents and frequently used files',
                'include': [],
                'exclude': []
            },
            'tier3': {
                'name': 'Archive Data',
                'description': 'Large files and archives, backed up less frequently',
                'include': [],
                'exclude': []
            }
        }
