import customtkinter as ctk
import tkinter as tk
from tkinter import filedialog, messagebox
import os
import sys
from data import DataManager

class EditableTable:
    """A helper class to manage editable table cells"""
    
    def __init__(self, parent, data, columns, on_cell_changed=None, on_selection_changed=None):
        self.parent = parent
        self.data = data
        self.columns = columns  # List of (display_name, width, data_key) tuples
        self.on_cell_changed = on_cell_changed
        self.on_selection_changed = on_selection_changed
        self.cells = {}
        self.checkboxes = {}
        self.current_edit = None
        
    def create_table(self):
        """Create the editable table"""
        # Create header with checkbox column first
        # Add checkbox header
        checkbox_header = ctk.CTkLabel(
            self.parent,
            text="✓",
            font=("Arial", 11, "bold"),
            anchor="center",
            width=40,
            fg_color="#4a6fa5",
            text_color="white"
        )
        checkbox_header.grid(row=0, column=0, padx=1, pady=1, sticky="nsew")
        
        # Create other headers starting from column 1
        for col_idx, (display_name, width, data_key) in enumerate(self.columns, 1):
            header = ctk.CTkLabel(
                self.parent,
                text=display_name,
                font=("Arial", 11, "bold"),
                anchor="center",
                width=width,
                fg_color="#4a6fa5",
                text_color="white"
            )
            header.grid(row=0, column=col_idx, padx=1, pady=1, sticky="nsew")
            self.parent.grid_columnconfigure(col_idx, weight=1)
        
        # Create data rows
        for row_idx, row_data in enumerate(self.data, 1):
            row_color = "#ffffff" if row_idx % 2 == 1 else "#f0f0f0"
            
            # Create checkbox for selection
            var = tk.BooleanVar(value=False)
            checkbox = ctk.CTkCheckBox(
                self.parent,
                text="",
                variable=var,
                width=40,
                height=20,
                command=lambda r=row_idx-1: self.on_checkbox_changed(r)
            )
            checkbox.grid(row=row_idx, column=0, padx=10, pady=1, sticky="nsew")
            self.checkboxes[row_idx-1] = var
            
            # Create data cells starting from column 1
            for col_idx, (display_name, width, data_key) in enumerate(self.columns, 1):
                cell_key = (row_idx - 1, data_key)
                value = row_data.get(data_key, '')
                
                # Create label that can be clicked to edit
                cell = ctk.CTkLabel(
                    self.parent,
                    text=str(value),
                    font=("Arial", 10),
                    anchor="w" if not str(value).replace('.', '').replace('-', '').isdigit() else "e",
                    width=width,
                    fg_color=row_color,
                    text_color="#000000"
                )
                cell.grid(row=row_idx, column=col_idx, padx=1, pady=1, sticky="nsew")
                
                self.cells[cell_key] = {
                    'widget': cell,
                    'value': value,
                    'row_color': row_color,
                    'data_key': data_key
                }
                
        # Configure checkbox column
        self.parent.grid_columnconfigure(0, weight=0)
    
    def get_selected_indices(self):
        """Get indices of selected rows"""
        selected = []
        for idx, var in self.checkboxes.items():
            if var.get():
                selected.append(idx)
        return selected
    
    def get_selected_data(self):
        """Get data for selected rows"""
        selected_indices = self.get_selected_indices()
        return [self.data[idx] for idx in selected_indices]
    
    def on_checkbox_changed(self, row_idx):
        """Handle checkbox change"""
        if self.on_selection_changed:
            self.on_selection_changed(self.get_selected_indices())
    
    def get_column_index(self, data_key):
        """Get column index by data key"""
        for idx, (display_name, width, key) in enumerate(self.columns):
            if key == data_key:
                return idx + 1  # +1 because checkbox is column 0
        return 1

class EditSelectedWindow(ctk.CTkToplevel):
    """Window for editing selected entries"""
    
    def __init__(self, parent, selected_data, on_save_callback):
        super().__init__(parent)
        
        self.title("Edit Selected Entries")
        self.geometry("1200x800")
        self.transient(parent)  # Make it modal to parent
        
        self.selected_data = selected_data
        self.on_save_callback = on_save_callback
        
        # Dictionary to store entry widgets
        self.entry_widgets = {}
        
        self.setup_ui()
        
        # Bind Escape key to close
        self.bind('<Escape>', lambda e: self.destroy())
        
        # Make window modal
        self.grab_set()
        self.focus_set()
        
    def setup_ui(self):
        """Setup the edit window UI"""
        # Header
        header_frame = ctk.CTkFrame(self)
        header_frame.pack(fill="x", padx=20, pady=20)
        
        header_label = ctk.CTkLabel(
            header_frame,
            text=f"Editing {len(self.selected_data)} Selected Entries",
            font=("Arial", 24, "bold")
        )
        header_label.pack(pady=10)
        
        # Instructions
        instruction_label = ctk.CTkLabel(
            header_frame,
            text="All fields are editable. Changes will be saved to teams.csv when you click 'Save Changes'.",
            font=("Arial", 14),
            text_color="yellow"
        )
        instruction_label.pack(pady=(0, 10))
        
        # Create scrollable container
        container_frame = ctk.CTkFrame(self)
        container_frame.pack(fill="both", expand=True, padx=20, pady=(0, 20))
        
        # Create canvas and scrollbars
        canvas = tk.Canvas(container_frame, bg="#f0f0f0", highlightthickness=0)
        scrollbar_y = ctk.CTkScrollbar(container_frame, orientation="vertical", command=canvas.yview)
        scrollbar_x = ctk.CTkScrollbar(container_frame, orientation="horizontal", command=canvas.xview)
        
        scrollable_frame = ctk.CTkFrame(canvas)
        scrollable_frame.configure(fg_color="#f0f0f0")
        
        # Configure canvas
        canvas.configure(yscrollcommand=scrollbar_y.set, xscrollcommand=scrollbar_x.set)
        
        # Pack everything
        scrollbar_y.pack(side="right", fill="y")
        scrollbar_x.pack(side="bottom", fill="x")
        canvas.pack(side="left", fill="both", expand=True)
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        
        # Configure scrolling
        scrollable_frame.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        
        # Create edit fields
        self.create_edit_fields(scrollable_frame)
        
        # Action buttons
        button_frame = ctk.CTkFrame(self)
        button_frame.pack(fill="x", padx=20, pady=(0, 20))
        
        # Save button
        save_button = ctk.CTkButton(
            button_frame,
            text="💾 Save Changes",
            command=self.save_changes,
            font=("Arial", 16),
            height=45,
            fg_color="green",
            hover_color="dark green",
            width=200
        )
        save_button.pack(side="right", padx=10)
        
        # Cancel button
        cancel_button = ctk.CTkButton(
            button_frame,
            text="Cancel",
            command=self.destroy,
            font=("Arial", 16),
            height=45,
            fg_color="gray",
            hover_color="dark gray",
            width=150
        )
        cancel_button.pack(side="right", padx=10)
        
    def create_edit_fields(self, parent):
        """Create editable fields for each selected entry"""
        if not self.selected_data:
            return
            
        # Get all unique keys from all entries
        all_keys = set()
        for entry in self.selected_data:
            all_keys.update(entry.keys())
        
        # Sort keys for consistent display
        sorted_keys = sorted(all_keys)
        
        # Create headers
        headers_frame = ctk.CTkFrame(parent)
        headers_frame.pack(fill="x", padx=10, pady=(10, 5))
        
        # Index column
        index_header = ctk.CTkLabel(
            headers_frame,
            text="#",
            font=("Arial", 11, "bold"),
            width=50,
            anchor="center"
        )
        index_header.pack(side="left", padx=2)
        
        # Data headers
        for key in sorted_keys:
            display_name = key.replace('RCD_', '') if key.startswith('RCD_') else key
            header = ctk.CTkLabel(
                headers_frame,
                text=display_name[:20],  # Truncate long names
                font=("Arial", 11, "bold"),
                width=120,
                anchor="center"
            )
            header.pack(side="left", padx=2)
        
        # Create entries for each row
        for idx, entry in enumerate(self.selected_data):
            row_frame = ctk.CTkFrame(parent)
            row_frame.pack(fill="x", padx=10, pady=2)
            
            # Background color alternating
            bg_color = "#ffffff" if idx % 2 == 0 else "#f0f0f0"
            row_frame.configure(fg_color=bg_color)
            
            # Index label
            index_label = ctk.CTkLabel(
                row_frame,
                text=str(idx + 1),
                font=("Arial", 10),
                width=50,
                anchor="center"
            )
            index_label.pack(side="left", padx=2)
            
            # Entry widgets for each key
            row_entries = {}
            for key in sorted_keys:
                value = entry.get(key, '')
                if isinstance(value, (int, float)):
                    value = str(value)
                
                entry_widget = ctk.CTkEntry(
                    row_frame,
                    font=("Arial", 10),
                    width=120,
                    fg_color="white",
                    text_color="black",
                    border_width=1,
                    border_color="#cccccc"
                )
                entry_widget.insert(0, value)
                entry_widget.pack(side="left", padx=2)
                
                row_entries[key] = entry_widget
            
            self.entry_widgets[idx] = {
                'original_data': entry.copy(),
                'entries': row_entries
            }
    
    def save_changes(self):
        """Save changes made in the edit window"""
        try:
            # Collect changes
            updated_entries = []
            for idx, widget_data in self.entry_widgets.items():
                updated_entry = widget_data['original_data'].copy()
                for key, entry_widget in widget_data['entries'].items():
                    new_value = entry_widget.get()
                    # Try to convert back to original type if possible
                    original_value = widget_data['original_data'].get(key, '')
                    if isinstance(original_value, int):
                        try:
                            new_value = int(new_value)
                        except:
                            pass  # Keep as string
                    elif isinstance(original_value, float):
                        try:
                            new_value = float(new_value)
                        except:
                            pass  # Keep as string
                    
                    updated_entry[key] = new_value
                updated_entries.append(updated_entry)
            
            # Call callback to save changes
            if self.on_save_callback:
                success = self.on_save_callback(self.selected_data, updated_entries)
                if success:
                    messagebox.showinfo("Success", "Changes saved successfully to teams.csv!")
                    self.destroy()
                else:
                    messagebox.showerror("Error", "Failed to save changes.")
            else:
                self.destroy()
                
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save changes: {str(e)}")

class GTR2TalentsTunerApp(ctk.CTk):
    def __init__(self, config_manager):
        super().__init__()
        
        self.title("GTR2 Talents Tuner")
        
        # Start in fullscreen mode
        self.attributes('-fullscreen', True)
        
        # Configuration
        self.config_manager = config_manager
        self.gtr2_path = config_manager.get_gtr2_path()
        self.teams_path = config_manager.get_teams_path()
        
        # Data manager - now passing gtr2_path for RCD search
        self.data_manager = DataManager(self.gtr2_path)
        
        # Selected folder
        self.selected_folder = tk.StringVar()
        self.selected_folder.set("No folder selected")
        
        # List to store car files
        self.car_files = []
        self.talents_data = []  # Store parsed talents data (one per driver)
        
        # Get program directory for CSV output
        self.program_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
        
        # Dictionary to hold all screens
        self.screens = {}
        
        # Current screen tracker
        self.current_screen = None
        
        # Editable table
        self.editable_table = None
        
        # Edit button reference
        self.edit_selected_button = None
        
        # Build UI
        self.setup_ui()
        
        # Show main screen first
        self.show_screen("main")
        
        # Bind Escape key to exit fullscreen
        self.bind('<Escape>', lambda e: self.toggle_fullscreen())
        
    def setup_ui(self):
        """Setup the main container and screens"""
        # Create main container that will hold all screens
        self.main_container = ctk.CTkFrame(self)
        self.main_container.pack(fill="both", expand=True)
        
        # Create all screens but don't show them yet
        self.create_main_screen()
        self.create_talents_table_screen()
        
    def create_main_screen(self):
        """Create the main/first screen"""
        self.screens["main"] = ctk.CTkFrame(self.main_container)
        
        # Title
        title_label = ctk.CTkLabel(
            self.screens["main"],
            text="GTR2 Talents Tuner",
            font=("Arial", 32, "bold")
        )
        title_label.pack(pady=(40, 30))
        
        # Configuration info frame
        config_frame = ctk.CTkFrame(self.screens["main"])
        config_frame.pack(fill="x", padx=50, pady=(0, 30))
        
        config_title = ctk.CTkLabel(
            config_frame,
            text="Configuration",
            font=("Arial", 20, "bold")
        )
        config_title.pack(pady=(20, 10))
        
        # Show paths
        program_dir_info = f"Program Directory: {self.program_dir}"
        gtr2_path_info = f"GTR2 Path: {self.gtr2_path}"
        teams_path_info = f"Teams Folder: {self.teams_path}"
        talent_path_info = f"Talent Folder: {os.path.join(self.gtr2_path, 'GameData', 'Talent') if self.gtr2_path else 'N/A'}"
        
        path_label = ctk.CTkLabel(
            config_frame,
            text=f"{program_dir_info}\n{gtr2_path_info}\n{teams_path_info}\n{talent_path_info}",
            font=("Arial", 14),
            wraplength=800,
            justify="left"
        )
        path_label.pack(pady=15, padx=30)
        
        # Folder selection section
        selection_frame = ctk.CTkFrame(self.screens["main"])
        selection_frame.pack(fill="x", padx=50, pady=(0, 30))
        
        selection_title = ctk.CTkLabel(
            selection_frame,
            text="Select Team Folder",
            font=("Arial", 24, "bold")
        )
        selection_title.pack(pady=(20, 15))
        
        # Instruction text
        instruction = "Browse to select a team folder from the Teams directory.\nAll .car files will be shown recursively from the selected folder.\nData will be organized by driver (one entry per driver/RCD file).\nAll RCD file variables will be extracted and displayed.\nteams.csv will be generated automatically with all data."
        instruction_label = ctk.CTkLabel(
            selection_frame,
            text=instruction,
            font=("Arial", 16),
            wraplength=800,
            justify="center"
        )
        instruction_label.pack(pady=(0, 20))
        
        # Browse button
        browse_button = ctk.CTkButton(
            selection_frame,
            text="Browse Team Folders",
            command=self.browse_folder,
            font=("Arial", 18),
            height=50,
            width=250
        )
        browse_button.pack(pady=20)
        
        # Selected folder display
        selected_frame = ctk.CTkFrame(selection_frame)
        selected_frame.pack(fill="x", padx=30, pady=20)
        
        selected_label = ctk.CTkLabel(
            selected_frame,
            text="Selected Folder:",
            font=("Arial", 18, "bold")
        )
        selected_label.pack(anchor="w", padx=20, pady=(15, 5))
        
        self.selected_display = ctk.CTkLabel(
            selected_frame,
            textvariable=self.selected_folder,
            font=("Arial", 16),
            wraplength=900,
            justify="left",
            anchor="w"
        )
        self.selected_display.pack(fill="x", padx=20, pady=(0, 15))
        
        # Status frame
        status_frame = ctk.CTkFrame(self.screens["main"])
        status_frame.pack(fill="x", padx=50, pady=(0, 20))
        
        self.status_label = ctk.CTkLabel(
            status_frame,
            text="Ready to browse folders",
            font=("Arial", 14),
            text_color="yellow"
        )
        self.status_label.pack(pady=15)
        
        # Bottom buttons frame
        bottom_frame = ctk.CTkFrame(self.screens["main"])
        bottom_frame.pack(fill="x", padx=50, pady=(0, 30))
        
        # Exit button
        exit_btn = ctk.CTkButton(
            bottom_frame,
            text="Exit Application",
            command=self.on_closing,
            font=("Arial", 16),
            height=45,
            fg_color="dark red",
            hover_color="red"
        )
        exit_btn.pack(side="right", padx=10, pady=10)
        
    def create_talents_table_screen(self):
        """Create the talents table screen (one entry per driver)"""
        self.screens["talents_table"] = ctk.CTkFrame(self.main_container)
        
        # Header with back button and title
        header_frame = ctk.CTkFrame(self.screens["talents_table"])
        header_frame.pack(fill="x", padx=30, pady=(30, 20))
        
        # Back button
        back_button = ctk.CTkButton(
            header_frame,
            text="← Back to Folder Selection",
            command=lambda: self.show_screen("main"),
            font=("Arial", 16),
            height=40,
            width=200,
            fg_color="blue",
            hover_color="dark blue"
        )
        back_button.pack(side="left", padx=10)
        
        # Title
        table_title = ctk.CTkLabel(
            header_frame,
            text="Talents Data Table (One Entry Per Driver)",
            font=("Arial", 28, "bold")
        )
        table_title.pack(side="left", padx=20, expand=True)
        
        # Selection actions frame
        selection_actions_frame = ctk.CTkFrame(self.screens["talents_table"])
        selection_actions_frame.pack(fill="x", padx=30, pady=(0, 10))
        
        # Edit selected button (initially hidden)
        self.edit_selected_button = ctk.CTkButton(
            selection_actions_frame,
            text="✏️ Edit Selected Entries",
            command=self.open_edit_selected_window,
            font=("Arial", 16),
            height=40,
            fg_color="orange",
            hover_color="dark orange",
            state="disabled"  # Disabled until at least one entry is selected
        )
        self.edit_selected_button.pack(side="right", padx=10)
        
        # Selection info label
        self.selection_info_label = ctk.CTkLabel(
            selection_actions_frame,
            text="No entries selected",
            font=("Arial", 14),
            text_color="gray"
        )
        self.selection_info_label.pack(side="right", padx=20)
        
        # CSV status frame
        csv_status_frame = ctk.CTkFrame(self.screens["talents_table"])
        csv_status_frame.pack(fill="x", padx=30, pady=(0, 10))
        
        self.csv_status_label = ctk.CTkLabel(
            csv_status_frame,
            text="teams.csv status: Not generated yet",
            font=("Arial", 14),
            text_color="yellow"
        )
        self.csv_status_label.pack(pady=5)
        
        # Info frame
        info_frame = ctk.CTkFrame(self.screens["talents_table"])
        info_frame.pack(fill="x", padx=30, pady=(0, 20))
        
        self.table_info_label = ctk.CTkLabel(
            info_frame,
            text=f"No data loaded",
            font=("Arial", 14),
            wraplength=1200
        )
        self.table_info_label.pack(pady=10)
        
        # Table container with scrollbars
        table_container = ctk.CTkFrame(self.screens["talents_table"])
        table_container.pack(fill="both", expand=True, padx=30, pady=(0, 20))
        
        # Create scrollable frame for the table
        self.table_canvas = tk.Canvas(table_container, bg="#f0f0f0", highlightthickness=0)
        scrollbar_y = ctk.CTkScrollbar(table_container, orientation="vertical", command=self.table_canvas.yview)
        scrollbar_x = ctk.CTkScrollbar(table_container, orientation="horizontal", command=self.table_canvas.xview)
        
        self.table_inner_frame = ctk.CTkFrame(self.table_canvas)
        self.table_inner_frame.configure(fg_color="#f0f0f0")
        
        # Configure the canvas
        self.table_canvas.configure(yscrollcommand=scrollbar_y.set, xscrollcommand=scrollbar_x.set)
        
        # Pack everything
        scrollbar_y.pack(side="right", fill="y")
        scrollbar_x.pack(side="bottom", fill="x")
        self.table_canvas.pack(side="left", fill="both", expand=True)
        
        # Create window inside canvas
        self.table_canvas.create_window((0, 0), window=self.table_inner_frame, anchor="nw")
        
        # Configure scrolling
        self.table_inner_frame.bind("<Configure>", self.on_frame_configure)
        
        # Action buttons at bottom
        action_frame = ctk.CTkFrame(self.screens["talents_table"])
        action_frame.pack(fill="x", padx=30, pady=(0, 30))
        
        # Return to main button
        return_main_btn = ctk.CTkButton(
            action_frame,
            text="Select Different Folder",
            command=lambda: self.show_screen("main"),
            font=("Arial", 16),
            height=45,
            fg_color="blue",
            hover_color="dark blue"
        )
        return_main_btn.pack(side="right", padx=10, pady=10)
        
    def on_selection_changed(self, selected_indices):
        """Handle selection changes in the table"""
        count = len(selected_indices)
        if count > 0:
            self.selection_info_label.configure(
                text=f"{count} entr{'y' if count == 1 else 'ies'} selected",
                text_color="light green"
            )
            self.edit_selected_button.configure(state="normal")
        else:
            self.selection_info_label.configure(
                text="No entries selected",
                text_color="gray"
            )
            self.edit_selected_button.configure(state="disabled")
    
    def open_edit_selected_window(self):
        """Open window to edit selected entries"""
        if not self.editable_table:
            return
            
        selected_data = self.editable_table.get_selected_data()
        if not selected_data:
            messagebox.showwarning("No Selection", "Please select at least one entry to edit.")
            return
        
        # Create and open edit window
        EditSelectedWindow(
            self,
            selected_data,
            on_save_callback=self.save_edited_data
        )
    
    def save_edited_data(self, original_data, updated_data):
        """Save edited data back to talents_data and CSV"""
        try:
            # Update talents_data with new values
            for original, updated in zip(original_data, updated_data):
                # Find the index of this entry in talents_data
                for idx, entry in enumerate(self.talents_data):
                    if entry.get('RCDPath') == original.get('RCDPath') and entry.get('Driver') == original.get('Driver'):
                        # Update the entry
                        self.talents_data[idx].update(updated)
                        break
            
            # Save to CSV
            self.auto_generate_csv()
            
            # Update table display
            self.update_table_display()
            
            return True
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save changes: {str(e)}")
            return False
        
    def show_screen(self, screen_name):
        """Show the specified screen and hide others"""
        # Hide current screen if exists
        if self.current_screen:
            self.screens[self.current_screen].pack_forget()
        
        # Show the requested screen
        self.screens[screen_name].pack(fill="both", expand=True)
        self.current_screen = screen_name
        
        # Update table if showing talents table screen
        if screen_name == "talents_table":
            self.update_table_display()
    
    def browse_folder(self):
        """Open file browser to select a folder - EXACT FLOW AS REQUESTED"""
        if not self.teams_path or not os.path.exists(self.teams_path):
            messagebox.showerror("Error", "Teams path not found. Please check configuration.")
            return
        
        try:
            folder_path = filedialog.askdirectory(
                title="Select Team Folder",
                initialdir=self.teams_path,
                mustexist=True
            )
            
            if folder_path:
                # Ensure the selected folder is within the teams path
                if os.path.commonpath([folder_path, self.teams_path]) == self.teams_path:
                    self.selected_folder.set(folder_path)
                    
                    # Update status
                    self.status_label.configure(
                        text="⏳ Step 1/5: Finding .car files...",
                        text_color="yellow"
                    )
                    self.update()
                    
                    # STEP 1: Find .car files
                    self.car_files = self.data_manager.find_car_files_recursive(folder_path)
                    
                    self.status_label.configure(
                        text=f"⏳ Step 2/5: Found {len(self.car_files)} .car files, parsing data...",
                        text_color="yellow"
                    )
                    self.update()
                    
                    # STEP 2: Process car files for talents data (includes RCD search)
                    self.talents_data = self.data_manager.process_car_files_for_talents(folder_path, self.car_files)
                    
                    self.status_label.configure(
                        text=f"⏳ Step 3/5: Organized into {len(self.talents_data)} driver entries, counting RCD files...",
                        text_color="yellow"
                    )
                    self.update()
                    
                    # Count RCD files found
                    rcd_found = sum(1 for entry in self.talents_data if entry['RCDExists'])
                    
                    self.status_label.configure(
                        text="⏳ Step 4/5: Saving everything to teams.csv...",
                        text_color="yellow"
                    )
                    self.update()
                    
                    # STEP 4: Save everything to CSV
                    self.auto_generate_csv()
                    
                    self.status_label.configure(
                        text="⏳ Step 5/5: Switching to table view...",
                        text_color="yellow"
                    )
                    self.update()
                    
                    # STEP 5: Show data on GUI
                    # Automatically switch to table view
                    self.show_screen("talents_table")
                    
                    # Update final status
                    self.status_label.configure(
                        text=f"✓ Complete! Found {len(self.car_files)} .car files, {len(self.talents_data)} driver entries ({rcd_found} RCD files)",
                        text_color="light green"
                    )
                    
                else:
                    messagebox.showwarning(
                        "Invalid Selection",
                        "Please select a folder from within the Teams directory."
                    )
            else:
                self.status_label.configure(
                    text="Folder selection cancelled",
                    text_color="yellow"
                )
                
        except Exception as e:
            messagebox.showerror("Error", f"Could not browse folder: {e}")
            self.status_label.configure(
                text="Error browsing folder",
                text_color="red"
            )
    
    def auto_generate_csv(self):
        """Automatically generate teams.csv file WITH RCD DATA"""
        if not self.talents_data:
            self.csv_status_label.configure(
                text="teams.csv status: No data to save",
                text_color="red"
            )
            return
        
        try:
            # Use program directory for output
            output_path = os.path.join(self.program_dir, "teams.csv")
            
            # Save CSV WITH RCD DATA - using save_teams_csv
            success, result, saved_path = self.data_manager.save_teams_csv(self.talents_data, self.program_dir)
            
            if success:
                # Count RCD columns in CSV
                rcd_cols = sum(1 for key in self.talents_data[0].keys() if key.startswith('RCD_')) if self.talents_data else 0
                self.csv_status_label.configure(
                    text=f"teams.csv status: ✓ Saved {result} driver entries ({rcd_cols} RCD variables each) to: {os.path.basename(saved_path)}",
                    text_color="light green"
                )
                
                # Verify CSV was written with RCD data
                if os.path.exists(saved_path):
                    with open(saved_path, 'r', encoding='utf-8') as f:
                        lines = f.readlines()
                        if len(lines) > 0:
                            headers = lines[0].strip().split(',')
                            rcd_headers = [h for h in headers if h.startswith('RCD_')]
                            print(f"teams.csv written with {len(rcd_headers)} RCD columns")
                            print(f"Sample RCD columns: {rcd_headers[:5]}")
                            
                            # Print sample data if available
                            if len(lines) > 1:
                                first_data = lines[1].strip().split(',')
                                print(f"First data row has {len(first_data)} columns")
            else:
                error_msg = f"teams.csv status: ✗ Failed to save: {result}"
                self.csv_status_label.configure(
                    text=error_msg,
                    text_color="red"
                )
                
        except Exception as e:
            error_msg = f"teams.csv status: ✗ Error generating CSV: {str(e)[:100]}"
            self.csv_status_label.configure(
                text=error_msg,
                text_color="red"
            )
    
    def update_table_display(self):
        """Update the data table with current talents data - REORDERED COLUMNS"""
        # Clear existing table content
        for widget in self.table_inner_frame.winfo_children():
            widget.destroy()
        
        if not self.talents_data:
            no_data_label = ctk.CTkLabel(
                self.table_inner_frame,
                text="No data loaded. Please select a folder first.",
                font=("Arial", 20),
                text_color="gray"
            )
            no_data_label.pack(pady=100)
            return
        
        # Update info label
        self.table_info_label.configure(
            text=f"Showing {len(self.talents_data)} driver entries from: {self.selected_folder.get()}"
        )
        
        # Reset selection info
        self.selection_info_label.configure(
            text="No entries selected",
            text_color="gray"
        )
        self.edit_selected_button.configure(state="disabled")
        
        # Define the ORIGINAL first 5 columns (these will move to the right)
        original_first_five = [
            ('Driver', 150, 'Driver'),
            ('RCD Path', 250, 'RCDPath'),
            ('CAR File', 120, 'CARFile'),
            ('Team', 120, 'Team'),
            ('Car #', 80, 'car_number')
        ]
        
        # Find all RCD columns in the data
        rcd_columns = []
        if self.talents_data:
            # Get RCD columns from first entry that has RCD data
            rcd_entry = next((entry for entry in self.talents_data if entry['RCDExists']), self.talents_data[0])
            for key in rcd_entry.keys():
                if key.startswith('RCD_'):
                    # Remove RCD_ prefix for display, keep original key for data access
                    display_name = key.replace('RCD_', '')
                    rcd_columns.append((display_name, 100, key))
        
        # Sort RCD columns by display name for consistent display
        rcd_columns.sort(key=lambda x: x[0])
        
        # REORDER COLUMNS: 
        # 1. First, check if "Abbreviation" exists and put it first
        # 2. Then all other RCD columns (except Abbreviation if we already added it)
        # 3. Finally, the original first 5 columns
        
        columns = []
        
        # Look for "Abbreviation" column (case-insensitive)
        abbreviation_column = None
        other_rcd_columns = []
        
        for display_name, width, data_key in rcd_columns:
            if display_name.lower() == 'abbreviation':
                abbreviation_column = (display_name, width, data_key)
            else:
                other_rcd_columns.append((display_name, width, data_key))
        
        # Add Abbreviation first if found
        if abbreviation_column:
            columns.append(abbreviation_column)
        
        # Add all other RCD columns
        columns.extend(other_rcd_columns)
        
        # Finally, add the original first 5 columns (moved to rightmost)
        columns.extend(original_first_five)
        
        # Create table with selection capability
        self.editable_table = EditableTable(
            self.table_inner_frame,
            self.talents_data,
            columns,
            on_cell_changed=None,  # Disabled for now
            on_selection_changed=self.on_selection_changed
        )
        self.editable_table.create_table()
        
        # Configure grid weights (starting from column 1 because column 0 is checkboxes)
        self.table_inner_frame.grid_columnconfigure(0, weight=0)  # Checkbox column
        for i in range(1, len(columns) + 1):
            self.table_inner_frame.grid_columnconfigure(i, weight=1)
    
    def on_frame_configure(self, event=None):
        """Configure the scrollable region"""
        self.table_canvas.configure(scrollregion=self.table_canvas.bbox("all"))
    
    def toggle_fullscreen(self):
        """Toggle fullscreen mode"""
        current_state = self.attributes('-fullscreen')
        self.attributes('-fullscreen', not current_state)
    
    def on_closing(self):
        """Handle window closing"""
        self.destroy()
