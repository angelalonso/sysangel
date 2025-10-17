import customtkinter as ctk
from tkinter import messagebox, filedialog
from config.config_manager import config_manager
import os
import logging

class TierDialog:
    """Handles the tier configuration dialog"""
    
    def __init__(self, parent, tier_name: str, on_tier_saved: callable):
        self.parent = parent
        self.tier_name = tier_name
        self.on_tier_saved = on_tier_saved
        self.logger = logging.getLogger(__name__)
        
        # Get current tier configuration
        tiers_config = config_manager.get('backup.tiers', self._get_default_tiers())
        self.tier_config = tiers_config.get(tier_name, {})
    
    def show(self):
        """Show the tier configuration dialog"""
        from utils.ui_utils import create_modal_dialog
        
        # Create dialog
        self.dialog = create_modal_dialog(self.parent, f"Configure {self.tier_config.get('name', 'Tier')}", 700, 600)
        
        # Configure dialog grid
        self.dialog.grid_rowconfigure(1, weight=1)
        self.dialog.grid_columnconfigure(0, weight=1)
        
        # Dialog content
        self._create_dialog_content()
        
        # Wait for the window to be closed
        self.parent.wait_window(self.dialog)
    
    def _create_dialog_content(self):
        """Create the dialog content"""
        # Title
        title_label = ctk.CTkLabel(self.dialog, 
                                  text=f"Configure {self.tier_config.get('name', 'Tier')}",
                                  font=("Arial", 16, "bold"))
        title_label.grid(row=0, column=0, sticky="w", padx=20, pady=(15, 10))
        
        # Main content frame
        content_frame = ctk.CTkFrame(self.dialog)
        content_frame.grid(row=1, column=0, sticky="nsew", padx=20, pady=10)
        content_frame.grid_rowconfigure(2, weight=1)
        content_frame.grid_rowconfigure(5, weight=1)
        content_frame.grid_columnconfigure(0, weight=1)
        content_frame.grid_columnconfigure(1, weight=1)
        
        # Tier name
        ctk.CTkLabel(content_frame, 
                    text="Tier Name:",
                    font=("Arial", 14)).grid(row=0, column=0, sticky="w", padx=10, pady=5)
        
        self.name_var = ctk.StringVar(value=self.tier_config.get('name', ''))
        name_entry = ctk.CTkEntry(content_frame,
                                 textvariable=self.name_var,
                                 font=("Arial", 14))
        name_entry.grid(row=0, column=1, sticky="ew", padx=10, pady=5)
        
        # Description
        ctk.CTkLabel(content_frame, 
                    text="Description:",
                    font=("Arial", 14)).grid(row=1, column=0, sticky="w", padx=10, pady=5)
        
        self.desc_var = ctk.StringVar(value=self.tier_config.get('description', ''))
        desc_entry = ctk.CTkEntry(content_frame,
                                 textvariable=self.desc_var,
                                 font=("Arial", 14))
        desc_entry.grid(row=1, column=1, sticky="ew", padx=10, pady=5)
        
        # Include list
        include_label = ctk.CTkLabel(content_frame, 
                                    text="Folders to Include:",
                                    font=("Arial", 14, "bold"))
        include_label.grid(row=2, column=0, columnspan=2, sticky="w", padx=10, pady=(15, 5))
        
        include_frame = ctk.CTkFrame(content_frame)
        include_frame.grid(row=3, column=0, columnspan=2, sticky="nsew", padx=10, pady=5)
        include_frame.grid_rowconfigure(0, weight=1)
        include_frame.grid_columnconfigure(0, weight=1)
        
        self.include_listbox = ctk.CTkTextbox(include_frame, wrap="none")
        self.include_listbox.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)
        
        # Populate include list
        for item in self.tier_config.get('include', []):
            self.include_listbox.insert("end", item + "\n")
        
        include_buttons_frame = ctk.CTkFrame(include_frame, fg_color="transparent")
        include_buttons_frame.grid(row=0, column=1, sticky="ns", padx=5, pady=5)
        
        ctk.CTkButton(include_buttons_frame,
                     text="Add Folder",
                     command=self._add_include_folder).pack(pady=2)
        ctk.CTkButton(include_buttons_frame,
                     text="Remove Selected",
                     command=self._remove_selected_include).pack(pady=2)
        ctk.CTkButton(include_buttons_frame,
                     text="Clear All",
                     command=self._clear_include).pack(pady=2)
        
        # Exclude list
        exclude_label = ctk.CTkLabel(content_frame, 
                                    text="Folders/Files to Exclude:",
                                    font=("Arial", 14, "bold"))
        exclude_label.grid(row=4, column=0, columnspan=2, sticky="w", padx=10, pady=(15, 5))
        
        exclude_frame = ctk.CTkFrame(content_frame)
        exclude_frame.grid(row=5, column=0, columnspan=2, sticky="nsew", padx=10, pady=5)
        exclude_frame.grid_rowconfigure(0, weight=1)
        exclude_frame.grid_columnconfigure(0, weight=1)
        
        self.exclude_listbox = ctk.CTkTextbox(exclude_frame, wrap="none")
        self.exclude_listbox.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)
        
        # Populate exclude list
        for item in self.tier_config.get('exclude', []):
            self.exclude_listbox.insert("end", item + "\n")
        
        exclude_buttons_frame = ctk.CTkFrame(exclude_frame, fg_color="transparent")
        exclude_buttons_frame.grid(row=0, column=1, sticky="ns", padx=5, pady=5)
        
        ctk.CTkButton(exclude_buttons_frame,
                     text="Add Item",
                     command=self._add_exclude_item).pack(pady=2)
        ctk.CTkButton(exclude_buttons_frame,
                     text="Remove Selected",
                     command=self._remove_selected_exclude).pack(pady=2)
        ctk.CTkButton(exclude_buttons_frame,
                     text="Clear All",
                     command=self._clear_exclude).pack(pady=2)
        
        # Buttons frame
        buttons_frame = ctk.CTkFrame(self.dialog, fg_color="transparent")
        buttons_frame.grid(row=2, column=0, sticky="e", padx=20, pady=15)
        
        save_btn = ctk.CTkButton(buttons_frame, 
                                text="Save Tier",
                                command=self._save_tier,
                                height=35)
        save_btn.pack(side="right", padx=(10, 0))
        
        cancel_btn = ctk.CTkButton(buttons_frame, 
                                  text="Cancel",
                                  command=self.dialog.destroy,
                                  height=35,
                                  fg_color="gray")
        cancel_btn.pack(side="right")
    
    def _add_include_folder(self):
        """Add folder to include list"""
        folder = filedialog.askdirectory(title="Select folder to include in backup")
        if folder:
            self.include_listbox.insert("end", folder + "\n")
    
    def _add_exclude_item(self):
        """Add item to exclude list"""
        # Let user choose between file and folder
        choice = messagebox.askyesno("Add Exclude Item", 
                                   "Select 'Yes' for a folder, 'No' for a file")
        if choice:
            item = filedialog.askdirectory(title="Select folder to exclude from backup")
        else:
            item = filedialog.askopenfilename(title="Select file to exclude from backup")
        
        if item:
            self.exclude_listbox.insert("end", item + "\n")
    
    def _remove_selected_include(self):
        """Remove selected item from include list"""
        try:
            # Get selected text
            selected = self.include_listbox.get("sel.first", "sel.last")
            if selected:
                # Remove the selected text
                self.include_listbox.delete("sel.first", "sel.last")
        except:
            messagebox.showwarning("No Selection", "Please select an item to remove")
    
    def _remove_selected_exclude(self):
        """Remove selected item from exclude list"""
        try:
            # Get selected text
            selected = self.exclude_listbox.get("sel.first", "sel.last")
            if selected:
                # Remove the selected text
                self.exclude_listbox.delete("sel.first", "sel.last")
        except:
            messagebox.showwarning("No Selection", "Please select an item to remove")
    
    def _clear_include(self):
        """Clear all include items"""
        if messagebox.askyesno("Clear All", "Clear all include items?"):
            self.include_listbox.delete("1.0", "end")
    
    def _clear_exclude(self):
        """Clear all exclude items"""
        if messagebox.askyesno("Clear All", "Clear all exclude items?"):
            self.exclude_listbox.delete("1.0", "end")
    
    def _save_tier(self):
        """Save tier configuration"""
        try:
            # Get include items
            include_text = self.include_listbox.get("1.0", "end-1c")
            include_items = [line.strip() for line in include_text.split('\n') if line.strip()]
            
            # Get exclude items
            exclude_text = self.exclude_listbox.get("1.0", "end-1c")
            exclude_items = [line.strip() for line in exclude_text.split('\n') if line.strip()]
            
            # Create tier configuration
            tier_config = {
                'name': self.name_var.get(),
                'description': self.desc_var.get(),
                'include': include_items,
                'exclude': exclude_items
            }
            
            # Validate
            if not tier_config['name']:
                messagebox.showerror("Error", "Tier name is required")
                return
            
            # Save via callback
            self.on_tier_saved(self.tier_name, tier_config)
            self.dialog.destroy()
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save tier: {str(e)}")
    
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
