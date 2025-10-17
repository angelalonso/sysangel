import customtkinter as ctk
from tkinter import messagebox, filedialog
import os
import logging
from config.config_manager import config_manager
from .base_page import BasePage
from utils.ui_utils import setup_scrollable_content, create_responsive_grid, create_section_header
from utils.media_utils import MediaDialogWithTiers

class ConfigurePage(BasePage):
    def setup_ui(self):
        # Set up logger for this class
        self.logger = logging.getLogger(__name__)
        
        # Configure the main frame to expand properly
        self.grid_rowconfigure(0, weight=1)
        self.grid_columnconfigure(0, weight=1)
        
        # Header with back button
        self._create_header()
        
        # Main content with scrolling
        self.main_content, self.scrollable_frame = setup_scrollable_content(self)
        
        # Configure scrollable frame grid
        create_responsive_grid(self.scrollable_frame, rows=12, cols=3)
        
        # Create all sections
        self._create_tier_section()
        self._create_media_section()
        self._create_management_section()
        self._create_status_section()
        
        # Load configured data on page show
        self.load_configured_data()
    
    def _create_header(self):
        """Create the page header"""
        self.header_frame = ctk.CTkFrame(self)
        self.header_frame.pack(fill="x", padx=10, pady=10)
        
        self.back_btn = ctk.CTkButton(self.header_frame, 
                                     text="← Back", 
                                     command=lambda: self.controller.show_page("HomePage"),
                                     width=100)
        self.back_btn.pack(side="left", padx=10, pady=10)
        
        self.title_label = ctk.CTkLabel(self.header_frame, 
                                       text="Basic Backup - Configuration", 
                                       font=("Arial", 20, "bold"))
        self.title_label.pack(side="left", padx=20, pady=10)
    
    def _create_tier_section(self):
        """Create backup tier configuration section"""
        # Section header
        create_section_header(self.scrollable_frame, "Backup Tiers:", 0, columnspan=3)
        
        # Description
        desc_label = ctk.CTkLabel(self.scrollable_frame,
                                 text="Configure three backup tiers with different data priorities:",
                                 font=("Arial", 12),
                                 text_color="gray70")
        desc_label.grid(row=1, column=0, columnspan=3, sticky="w", padx=10, pady=(0, 15))
        
        # Tiers container
        self.tiers_container = ctk.CTkFrame(self.scrollable_frame)
        self.tiers_container.grid(row=2, column=0, columnspan=3, sticky="ew", padx=10, pady=(0, 15))
        self.tiers_container.grid_columnconfigure(0, weight=1)
        self.tiers_container.grid_columnconfigure(1, weight=1)
        self.tiers_container.grid_columnconfigure(2, weight=1)
    
    def _create_media_section(self):
        """Create media settings section"""
        # Section header
        create_section_header(self.scrollable_frame, "Backup Media:", 3, columnspan=3)
        
        # Description
        desc_label = ctk.CTkLabel(self.scrollable_frame,
                                 text="Configure media locations and assign backup tiers:",
                                 font=("Arial", 12),
                                 text_color="gray70")
        desc_label.grid(row=4, column=0, columnspan=3, sticky="w", padx=10, pady=(0, 10))
        
        # Add Media button
        self.media_btn = ctk.CTkButton(self.scrollable_frame, 
                                      text="➕ Add Backup Media",
                                      command=self.show_add_media_dialog,
                                      height=40,
                                      font=("Arial", 14))
        self.media_btn.grid(row=5, column=0, columnspan=3, sticky="ew", padx=10, pady=(0, 10))
        
        # Configured Media frame
        self.configured_media_frame = ctk.CTkFrame(self.scrollable_frame)
        self.configured_media_frame.grid(row=6, column=0, columnspan=3, sticky="ew", padx=10, pady=(0, 10))
        self.configured_media_frame.grid_columnconfigure(0, weight=1)
        
        self.configured_media_label = ctk.CTkLabel(self.configured_media_frame, 
                                                  text="Configured Backup Media:",
                                                  font=("Arial", 14, "bold"))
        self.configured_media_label.grid(row=0, column=0, sticky="w", padx=10, pady=(10, 5))
        
        # Container for configured media items
        self.configured_media_container = ctk.CTkFrame(self.configured_media_frame, fg_color="transparent")
        self.configured_media_container.grid(row=1, column=0, sticky="ew", padx=10, pady=(0, 10))
        self.configured_media_container.grid_columnconfigure(0, weight=1)
    
    def _create_management_section(self):
        """Create configuration management section"""
        # Section header
        create_section_header(self.scrollable_frame, "Configuration Management:", 7, columnspan=3)
        
        # Buttons frame
        self.management_frame = ctk.CTkFrame(self.scrollable_frame)
        self.management_frame.grid(row=8, column=0, columnspan=3, sticky="ew", padx=10, pady=10)
        
        # Use pack for buttons to handle wrapping better
        self.export_btn = ctk.CTkButton(self.management_frame, 
                                       text="Export Config",
                                       command=self.export_config)
        self.export_btn.pack(side="left", padx=5, pady=5, fill="x", expand=True)
        
        self.import_btn = ctk.CTkButton(self.management_frame, 
                                       text="Import Config",
                                       command=self.import_config)
        self.import_btn.pack(side="left", padx=5, pady=5, fill="x", expand=True)
        
        self.reset_btn = ctk.CTkButton(self.management_frame, 
                                      text="Reset to Defaults",
                                      command=self.reset_config,
                                      fg_color="orange",
                                      hover_color="dark orange")
        self.reset_btn.pack(side="left", padx=5, pady=5, fill="x", expand=True)
    
    def _create_status_section(self):
        """Create status section"""
        self.status_label = ctk.CTkLabel(self.scrollable_frame, 
                                        text="",
                                        text_color="green")
        self.status_label.grid(row=9, column=0, columnspan=3, sticky="w", padx=10, pady=10)
    
    def load_configured_data(self):
        """Load both tiers and media configuration"""
        self.load_tiers_configuration()
        self.load_configured_media()
    
    def load_tiers_configuration(self):
        """Load and display tier configuration"""
        # Clear existing tier widgets
        for widget in self.tiers_container.winfo_children():
            widget.destroy()
        
        # Get tier configuration
        tiers_config = config_manager.get('backup.tiers', self._get_default_tiers())
        
        # Create tier frames
        for i, (tier_name, tier_config) in enumerate(tiers_config.items()):
            tier_frame = ctk.CTkFrame(self.tiers_container)
            tier_frame.grid(row=0, column=i, sticky="nsew", padx=5, pady=5)
            tier_frame.grid_columnconfigure(0, weight=1)
            
            # Tier header
            tier_header = ctk.CTkFrame(tier_frame, fg_color="transparent")
            tier_header.grid(row=0, column=0, sticky="ew", padx=10, pady=(10, 5))
            
            tier_label = ctk.CTkLabel(tier_header, 
                                     text=f"Tier {i+1}: {tier_config['name']}",
                                     font=("Arial", 14, "bold"))
            tier_label.pack(side="left")
            
            # Edit button
            edit_btn = ctk.CTkButton(tier_header,
                                   text="✎",
                                   width=30,
                                   height=30,
                                   command=lambda t=tier_name: self.edit_tier_configuration(t))
            edit_btn.pack(side="right")
            
            # Description
            desc_label = ctk.CTkLabel(tier_frame,
                                     text=tier_config['description'],
                                     font=("Arial", 11),
                                     text_color="gray70",
                                     wraplength=200)
            desc_label.grid(row=1, column=0, sticky="w", padx=10, pady=(0, 5))
            
            # Include list preview
            include_label = ctk.CTkLabel(tier_frame,
                                       text=f"Includes: {len(tier_config['include'])} items",
                                       font=("Arial", 10))
            include_label.grid(row=2, column=0, sticky="w", padx=10, pady=2)
            
            # Exclude list preview
            exclude_label = ctk.CTkLabel(tier_frame,
                                       text=f"Excludes: {len(tier_config['exclude'])} items",
                                       font=("Arial", 10))
            exclude_label.grid(row=3, column=0, sticky="w", padx=10, pady=2)
    
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
    
    def edit_tier_configuration(self, tier_name):
        """Open dialog to edit tier configuration"""
        from utils.tier_utils import TierDialog
        
        tier_dialog = TierDialog(self, tier_name, self.save_tier_configuration)
        tier_dialog.show()
    
    def save_tier_configuration(self, tier_name, tier_config):
        """Save tier configuration"""
        try:
            # Get current tier configuration
            tiers_config = config_manager.get('backup.tiers', self._get_default_tiers())
            
            # Update the specific tier
            tiers_config[tier_name] = tier_config
            config_manager.set('backup.tiers', tiers_config)
            
            # Refresh the display
            self.load_tiers_configuration()
            
            self.show_status(f"Tier {tier_name} configuration updated")
            self.logger.info(f"Updated tier configuration: {tier_name}")
            
        except Exception as e:
            error_msg = f"Error saving tier configuration: {str(e)}"
            self.show_status(error_msg)
            self.logger.error(error_msg)
    
    def show_add_media_dialog(self):
        """Show dialog to select and add backup media"""
        media_dialog = MediaDialogWithTiers(self, self.add_media_to_config)
        media_dialog.show()
    
    def add_media_to_config(self, media_path, selected_tiers):
        """Add media path to configuration with assigned tiers"""
        try:
            # Get current configured media
            configured_media = config_manager.get('backup.media', [])
            
            # Check if already exists (handle both old and new formats)
            for i, media in enumerate(configured_media):
                if isinstance(media, dict) and media.get('path') == media_path:
                    # Update existing media (new format)
                    configured_media[i] = {'path': media_path, 'tiers': selected_tiers}
                    config_manager.set('backup.media', configured_media)
                    self.load_configured_media()
                    self.show_status(f"Media updated: {media_path}")
                    return
                elif isinstance(media, str) and media == media_path:
                    # Convert old format to new format
                    configured_media[i] = {'path': media_path, 'tiers': selected_tiers}
                    config_manager.set('backup.media', configured_media)
                    self.load_configured_media()
                    self.show_status(f"Media updated: {media_path}")
                    return
            
            # Add new media
            new_media = {
                'path': media_path,
                'tiers': selected_tiers
            }
            configured_media.append(new_media)
            config_manager.set('backup.media', configured_media)
            
            # Refresh the display
            self.load_configured_media()
            
            self.show_status(f"Media added: {media_path}")
            self.logger.info(f"Added media: {media_path} with tiers: {selected_tiers}")
            
        except Exception as e:
            error_msg = f"Error adding media: {str(e)}"
            self.show_status(error_msg)
            self.logger.error(error_msg)
    
    def remove_media_from_config(self, media_path):
        """Remove media path from configuration"""
        try:
            # Get current configured media
            configured_media = config_manager.get('backup.media', [])
            
            # Remove the media (handle both old and new formats)
            new_configured_media = []
            for media in configured_media:
                if isinstance(media, dict) and media.get('path') == media_path:
                    continue  # Skip this one (remove it)
                elif isinstance(media, str) and media == media_path:
                    continue  # Skip this one (remove it)
                else:
                    new_configured_media.append(media)
            
            config_manager.set('backup.media', new_configured_media)
            
            # Refresh the display
            self.load_configured_media()
            
            self.show_status(f"Media removed: {media_path}")
            self.logger.info(f"Removed media: {media_path}")
            
        except Exception as e:
            error_msg = f"Error removing media: {str(e)}"
            self.show_status(error_msg)
            self.logger.error(error_msg)
    
    def load_configured_media(self):
        """Load and display configured media with tier assignments"""
        self.logger.info("Loading configured media")
        
        # Clear existing media widgets
        for widget in self.configured_media_container.winfo_children():
            widget.destroy()
        
        # Get configured media from config
        configured_media = config_manager.get('backup.media', [])
        
        if not configured_media:
            # Show message when no media configured
            no_media_label = ctk.CTkLabel(self.configured_media_container,
                                         text="No backup media configured. Click 'Add Backup Media' to get started.",
                                         font=("Arial", 12),
                                         text_color="gray60")
            no_media_label.pack(pady=20)
            return
        
        # Get tier names for display
        tiers_config = config_manager.get('backup.tiers', self._get_default_tiers())
        tier_names = {tier_id: config['name'] for tier_id, config in tiers_config.items()}
        
        # Display each configured media item
        for media in configured_media:
            # Handle both old (string) and new (dict) media configurations
            if isinstance(media, str):
                # Old format: media is just a path string
                media_path = media
                assigned_tiers = []
            else:
                # New format: media is a dictionary
                media_path = media.get('path', '')
                assigned_tiers = media.get('tiers', [])
            
            media_item_frame = ctk.CTkFrame(self.configured_media_container)
            media_item_frame.pack(fill="x", pady=5)
            media_item_frame.grid_columnconfigure(0, weight=1)
            
            # Media path and tiers
            info_frame = ctk.CTkFrame(media_item_frame, fg_color="transparent")
            info_frame.grid(row=0, column=0, sticky="ew", padx=10, pady=5)
            info_frame.grid_columnconfigure(0, weight=1)
            
            # Path
            path_label = ctk.CTkLabel(info_frame,
                                     text=media_path,
                                     font=("Courier New", 11),
                                     anchor="w")
            path_label.grid(row=0, column=0, sticky="w")
            
            # Assigned tiers
            assigned_tier_names = [tier_names.get(tier, tier) for tier in assigned_tiers]
            if assigned_tier_names:
                tiers_label = ctk.CTkLabel(info_frame,
                                          text=f"Tiers: {', '.join(assigned_tier_names)}",
                                          font=("Arial", 10),
                                          text_color="gray70")
                tiers_label.grid(row=1, column=0, sticky="w", pady=(2, 0))
            
            # Edit and Remove buttons
            button_frame = ctk.CTkFrame(media_item_frame, fg_color="transparent")
            button_frame.grid(row=0, column=1, sticky="e", padx=5, pady=5)
            
            edit_btn = ctk.CTkButton(button_frame,
                                   text="✎",
                                   width=30,
                                   height=30,
                                   command=lambda p=media_path: self.edit_media_tiers(p))
            edit_btn.pack(side="left", padx=2)
            
            remove_btn = ctk.CTkButton(button_frame,
                                      text="✕",
                                      width=30,
                                      height=30,
                                      fg_color="#d9534f",
                                      hover_color="#c9302c",
                                      command=lambda p=media_path: self.remove_media_from_config(p))
            remove_btn.pack(side="left", padx=2)
        
        self.logger.info(f"Displayed {len(configured_media)} configured media items")
    
    def edit_media_tiers(self, media_path):
        """Edit tier assignments for existing media"""
        # Find the media configuration
        configured_media = config_manager.get('backup.media', [])
        media_config = None
        
        for media in configured_media:
            if isinstance(media, dict) and media.get('path') == media_path:
                media_config = media
                break
            elif isinstance(media, str) and media == media_path:
                # Convert old format to new format
                media_config = {'path': media_path, 'tiers': []}
                break
        
        if media_config:
            assigned_tiers = media_config.get('tiers', []) if isinstance(media_config, dict) else []
            media_dialog = MediaDialogWithTiers(self, self.add_media_to_config, 
                                              media_path, assigned_tiers)
            media_dialog.show()
    
    def export_config(self):
        """Export configuration to file"""
        try:
            file_path = filedialog.asksaveasfilename(
                defaultextension=".yaml",
                filetypes=[("YAML files", "*.yaml"), ("All files", "*.*")],
                title="Export configuration to..."
            )
            
            if file_path:
                config_manager.export_config(file_path)
                self.show_status(f"Configuration exported to {os.path.basename(file_path)}")
                
        except Exception as e:
            messagebox.showerror("Export Error", f"Failed to export configuration: {e}")
    
    def import_config(self):
        """Import configuration from file"""
        try:
            file_path = filedialog.askopenfilename(
                filetypes=[("YAML files", "*.yaml"), ("All files", "*.*")],
                title="Import configuration from..."
            )
            
            if file_path:
                config_manager.import_config(file_path)
                self.load_current_config()
                
                self.show_status("Configuration imported successfully!")
                
        except Exception as e:
            messagebox.showerror("Import Error", f"Failed to import configuration: {e}")
    
    def reset_config(self):
        """Reset configuration to defaults"""
        if messagebox.askyesno("Confirm Reset", "Are you sure you want to reset all settings to defaults?"):
            config_manager.reset_to_defaults()
            self.load_current_config()
            self.show_status("Configuration reset to defaults")
    
    def load_current_config(self):
        """Load current configuration into UI"""
        # Load configured media
        self.load_configured_media()
    
    def show_status(self, message: str):
        """Show status message"""
        self.status_label.configure(text=message)
        self.after(3000, lambda: self.status_label.configure(text=""))
    
    def on_page_show(self):
        """Called when page is shown - refresh configuration and media"""
        self.load_current_config()
        self.load_configured_media()
        self.show_status("Configuration and media list loaded")
