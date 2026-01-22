import customtkinter as ctk
import tkinter as tk
from tkinter import filedialog, messagebox
import os
import sys
from data import DataManager

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
        
        # Data manager
        self.data_manager = DataManager()
        
        # Selected folder
        self.selected_folder = tk.StringVar()
        self.selected_folder.set("No folder selected")
        
        # List to store car files
        self.car_files = []
        self.teams_data = []  # Store parsed teams data
        
        # Get program directory for CSV output
        self.program_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
        
        # Dictionary to hold all screens
        self.screens = {}
        
        # Current screen tracker
        self.current_screen = None
        
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
        self.create_data_table_screen()
        
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
        
        path_label = ctk.CTkLabel(
            config_frame,
            text=f"{program_dir_info}\n{gtr2_path_info}\n{teams_path_info}",
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
        instruction = "Browse to select a team folder from the Teams directory.\nAll .car files will be shown recursively from the selected folder.\nThe table view will open automatically and teams.csv will be generated."
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
        
    def create_data_table_screen(self):
        """Create the data table screen"""
        self.screens["data_table"] = ctk.CTkFrame(self.main_container)
        
        # Header with back button and title
        header_frame = ctk.CTkFrame(self.screens["data_table"])
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
            text="Car Files Data Table",
            font=("Arial", 28, "bold")
        )
        table_title.pack(side="left", padx=20, expand=True)
        
        # Refresh button
        refresh_button = ctk.CTkButton(
            header_frame,
            text="Refresh Table",
            command=self.refresh_table,
            font=("Arial", 16),
            height=40,
            width=150,
            fg_color="green",
            hover_color="dark green"
        )
        refresh_button.pack(side="right", padx=10)
        
        # CSV status frame
        csv_status_frame = ctk.CTkFrame(self.screens["data_table"])
        csv_status_frame.pack(fill="x", padx=30, pady=(0, 10))
        
        self.csv_status_label = ctk.CTkLabel(
            csv_status_frame,
            text="CSV status: Not generated yet",
            font=("Arial", 14),
            text_color="yellow"
        )
        self.csv_status_label.pack(pady=5)
        
        # Info frame
        info_frame = ctk.CTkFrame(self.screens["data_table"])
        info_frame.pack(fill="x", padx=30, pady=(0, 20))
        
        self.table_info_label = ctk.CTkLabel(
            info_frame,
            text=f"No data loaded",
            font=("Arial", 14),
            wraplength=1200
        )
        self.table_info_label.pack(pady=10)
        
        # Table container with scrollbars
        table_container = ctk.CTkFrame(self.screens["data_table"])
        table_container.pack(fill="both", expand=True, padx=30, pady=(0, 20))
        
        # Create scrollable frame for the table
        self.table_canvas = tk.Canvas(table_container, bg="#2b2b2b")
        scrollbar_y = ctk.CTkScrollbar(table_container, orientation="vertical", command=self.table_canvas.yview)
        scrollbar_x = ctk.CTkScrollbar(table_container, orientation="horizontal", command=self.table_canvas.xview)
        
        self.table_frame = ctk.CTkFrame(self.table_canvas)
        
        # Configure the canvas
        self.table_canvas.configure(yscrollcommand=scrollbar_y.set, xscrollcommand=scrollbar_x.set)
        
        # Pack everything
        scrollbar_y.pack(side="right", fill="y")
        scrollbar_x.pack(side="bottom", fill="x")
        self.table_canvas.pack(side="left", fill="both", expand=True)
        
        # Create window inside canvas
        self.table_canvas.create_window((0, 0), window=self.table_frame, anchor="nw")
        
        # Configure scrolling
        self.table_frame.bind("<Configure>", self.on_frame_configure)
        
        # Action buttons at bottom
        action_frame = ctk.CTkFrame(self.screens["data_table"])
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
        
    def show_screen(self, screen_name):
        """Show the specified screen and hide others"""
        # Hide current screen if exists
        if self.current_screen:
            self.screens[self.current_screen].pack_forget()
        
        # Show the requested screen
        self.screens[screen_name].pack(fill="both", expand=True)
        self.current_screen = screen_name
        
        # Update table if showing data table screen
        if screen_name == "data_table":
            self.update_table_display()
    
    def browse_folder(self):
        """Open file browser to select a folder"""
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
                    
                    # Find car files immediately using data manager
                    self.car_files = self.data_manager.find_car_files_recursive(folder_path)
                    
                    # Process car files for teams data
                    self.teams_data = self.data_manager.process_car_files_for_teams(folder_path, self.car_files)
                    
                    # Automatically generate CSV
                    self.auto_generate_csv()
                    
                    # Automatically switch to table view
                    self.show_screen("data_table")
                    
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
        """Automatically generate teams.csv file"""
        if not self.teams_data:
            self.csv_status_label.configure(
                text="CSV status: No data to save",
                text_color="red"
            )
            return
        
        try:
            # Use program directory for output
            output_path = os.path.join(self.program_dir, "teams.csv")
            
            # Save CSV
            success, result, saved_path = self.data_manager.save_teams_csv(self.teams_data, self.program_dir)
            
            if success:
                self.csv_status_label.configure(
                    text=f"CSV status: ✓ Automatically saved {result} entries to: {os.path.basename(saved_path)}",
                    text_color="light green"
                )
            else:
                error_msg = f"CSV status: ✗ Failed to save: {result}"
                self.csv_status_label.configure(
                    text=error_msg,
                    text_color="red"
                )
                
        except Exception as e:
            error_msg = f"CSV status: ✗ Error generating CSV: {str(e)[:100]}"
            self.csv_status_label.configure(
                text=error_msg,
                text_color="red"
            )
    
    def update_table_display(self):
        """Update the data table with current teams data"""
        # Clear existing table content
        for widget in self.table_frame.winfo_children():
            widget.destroy()
        
        if not self.teams_data:
            no_data_label = ctk.CTkLabel(
                self.table_frame,
                text="No data loaded. Please select a folder first.",
                font=("Arial", 20),
                text_color="gray"
            )
            no_data_label.pack(pady=100)
            return
        
        # Update info label
        self.table_info_label.configure(
            text=f"Showing {len(self.teams_data)} entries from: {self.selected_folder.get()}"
        )
        
        # Define column widths
        col_widths = {
            'Filename': 150,
            'Driver': 120,
            'Driver1': 120,
            'Driver2': 120,
            'Description': 200,
            'Team': 150,
            'Number': 80,
            'RelativePath': 250
        }
        
        # Create header row
        header_frame = ctk.CTkFrame(self.table_frame)
        header_frame.pack(fill="x", pady=(0, 5))
        
        # Create header labels
        headers = ['#', 'Filename', 'Driver', 'Driver1', 'Driver2', 'Description', 'Team', 'Number', 'Relative Path']
        
        for i, header in enumerate(headers):
            header_label = ctk.CTkLabel(
                header_frame,
                text=header,
                font=("Arial", 14, "bold"),
                anchor="w",
                width=col_widths.get(header, 100) if i > 0 else 50
            )
            header_label.grid(row=0, column=i, padx=5, pady=5, sticky="ew")
        
        # Configure header frame grid
        for i in range(len(headers)):
            header_frame.grid_columnconfigure(i, weight=1)
        
        # Add separator
        separator = ctk.CTkFrame(self.table_frame, height=2, fg_color="gray")
        separator.pack(fill="x", pady=(0, 10))
        
        # Create data rows
        for idx, team_data in enumerate(self.teams_data, 1):
            row_frame = ctk.CTkFrame(self.table_frame)
            row_frame.pack(fill="x", pady=2)
            
            # Create alternating background
            if idx % 2 == 0:
                row_frame.configure(fg_color="#2a2a2a")
            
            # Row number
            num_label = ctk.CTkLabel(
                row_frame,
                text=str(idx),
                font=("Arial", 12),
                anchor="w",
                width=50
            )
            num_label.grid(row=0, column=0, padx=5, pady=3, sticky="w")
            
            # Filename
            filename_label = ctk.CTkLabel(
                row_frame,
                text=team_data['Filename'],
                font=("Arial", 12),
                anchor="w",
                width=col_widths['Filename']
            )
            filename_label.grid(row=0, column=1, padx=5, pady=3, sticky="w")
            
            # Driver
            driver_text = team_data['Driver'] or "(empty)"
            driver_label = ctk.CTkLabel(
                row_frame,
                text=driver_text,
                font=("Arial", 12),
                anchor="w",
                width=col_widths['Driver']
            )
            driver_label.grid(row=0, column=2, padx=5, pady=3, sticky="w")
            
            # Driver1
            driver1_label = ctk.CTkLabel(
                row_frame,
                text=team_data['Driver1'],
                font=("Arial", 12),
                anchor="w",
                width=col_widths['Driver1']
            )
            driver1_label.grid(row=0, column=3, padx=5, pady=3, sticky="w")
            
            # Driver2
            driver2_label = ctk.CTkLabel(
                row_frame,
                text=team_data['Driver2'],
                font=("Arial", 12),
                anchor="w",
                width=col_widths['Driver2']
            )
            driver2_label.grid(row=0, column=4, padx=5, pady=3, sticky="w")
            
            # Description (truncate if too long)
            desc_text = team_data['Description']
            if len(desc_text) > 30:
                desc_text = desc_text[:27] + "..."
            desc_label = ctk.CTkLabel(
                row_frame,
                text=desc_text,
                font=("Arial", 12),
                anchor="w",
                width=col_widths['Description']
            )
            desc_label.grid(row=0, column=5, padx=5, pady=3, sticky="w")
            
            # Team
            team_label = ctk.CTkLabel(
                row_frame,
                text=team_data['Team'],
                font=("Arial", 12),
                anchor="w",
                width=col_widths['Team']
            )
            team_label.grid(row=0, column=6, padx=5, pady=3, sticky="w")
            
            # Number
            number_label = ctk.CTkLabel(
                row_frame,
                text=team_data['Number'],
                font=("Arial", 12),
                anchor="w",
                width=col_widths['Number']
            )
            number_label.grid(row=0, column=7, padx=5, pady=3, sticky="w")
            
            # Relative Path
            path_label = ctk.CTkLabel(
                row_frame,
                text=team_data['RelativePath'],
                font=("Arial", 12),
                anchor="w",
                width=col_widths['RelativePath']
            )
            path_label.grid(row=0, column=8, padx=5, pady=3, sticky="w")
            
            # Configure row grid
            for i in range(len(headers)):
                row_frame.grid_columnconfigure(i, weight=1)
    
    def on_frame_configure(self, event=None):
        """Configure the scrollable region"""
        self.table_canvas.configure(scrollregion=self.table_canvas.bbox("all"))
    
    def refresh_table(self):
        """Refresh the table display"""
        if self.selected_folder.get() and self.selected_folder.get() != "No folder selected":
            # Reload data
            folder_path = self.selected_folder.get()
            self.car_files = self.data_manager.find_car_files_recursive(folder_path)
            self.teams_data = self.data_manager.process_car_files_for_teams(folder_path, self.car_files)
            
            # Automatically regenerate CSV
            self.auto_generate_csv()
            
            # Update table
            self.update_table_display()
            
            # Show brief confirmation
            self.csv_status_label.configure(
                text=f"CSV status: ✓ Refreshed and saved {len(self.teams_data)} entries",
                text_color="light green"
            )
            
            # Brief status update (will clear after 3 seconds)
            original_text = self.csv_status_label.cget("text")
            self.after(3000, lambda: self.csv_status_label.configure(text=original_text))
        else:
            messagebox.showwarning("No Data", "No folder selected. Please select a folder first.")
    
    def toggle_fullscreen(self):
        """Toggle fullscreen mode"""
        current_state = self.attributes('-fullscreen')
        self.attributes('-fullscreen', not current_state)
    
    def on_closing(self):
        """Handle window closing"""
        self.destroy()
