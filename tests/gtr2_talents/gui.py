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
        self.geometry("1000x800")  # Increased size for more data display
        
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
        
        # Build UI
        self.setup_ui()
    
    def setup_ui(self):
        """Setup the user interface"""
        # Create main container with padding
        main_container = ctk.CTkFrame(self)
        main_container.pack(fill="both", expand=True, padx=20, pady=20)
        
        # Title
        title_label = ctk.CTkLabel(
            main_container,
            text="GTR2 Talents Tuner",
            font=("Arial", 28, "bold")
        )
        title_label.pack(pady=(0, 20))
        
        # Configuration info frame
        config_frame = ctk.CTkFrame(main_container)
        config_frame.pack(fill="x", pady=(0, 20))
        
        config_title = ctk.CTkLabel(
            config_frame,
            text="Configuration",
            font=("Arial", 16, "bold")
        )
        config_title.pack(pady=(10, 5))
        
        # Show paths
        program_dir_info = f"Program Directory: {self.program_dir}"
        gtr2_path_info = f"GTR2 Path: {self.gtr2_path}"
        teams_path_info = f"Teams Folder: {self.teams_path}"
        
        path_label = ctk.CTkLabel(
            config_frame,
            text=f"{program_dir_info}\n{gtr2_path_info}\n{teams_path_info}",
            font=("Arial", 12),
            wraplength=600,
            justify="left"
        )
        path_label.pack(pady=10, padx=20)
        
        # Folder selection section
        selection_frame = ctk.CTkFrame(main_container)
        selection_frame.pack(fill="both", expand=True)
        
        selection_title = ctk.CTkLabel(
            selection_frame,
            text="Select Team Folder",
            font=("Arial", 18, "bold")
        )
        selection_title.pack(pady=(15, 10))
        
        # Instruction text
        instruction = "Browse to select a team folder from the Teams directory.\nYou can navigate recursively through subdirectories.\nAll .car files will be shown recursively from the selected folder.\nteams.csv will be created in the program directory with:\nDriver, Driver1, Driver2, Description, Team, and Number fields."
        instruction_label = ctk.CTkLabel(
            selection_frame,
            text=instruction,
            font=("Arial", 12),
            wraplength=600,
            justify="center"
        )
        instruction_label.pack(pady=(0, 20))
        
        # Browse button
        browse_button = ctk.CTkButton(
            selection_frame,
            text="Browse Team Folders",
            command=self.browse_folder,
            font=("Arial", 14),
            height=40,
            width=200
        )
        browse_button.pack(pady=20)
        
        # Selected folder display
        selected_frame = ctk.CTkFrame(selection_frame)
        selected_frame.pack(fill="x", padx=50, pady=20)
        
        selected_label = ctk.CTkLabel(
            selected_frame,
            text="Selected Folder:",
            font=("Arial", 14, "bold")
        )
        selected_label.pack(anchor="w", padx=10, pady=(10, 5))
        
        self.selected_display = ctk.CTkLabel(
            selected_frame,
            textvariable=self.selected_folder,
            font=("Arial", 12),
            wraplength=700,
            justify="left",
            anchor="w"
        )
        self.selected_display.pack(fill="x", padx=10, pady=(0, 10))
        
        # Car files display area
        car_files_frame = ctk.CTkFrame(main_container)
        car_files_frame.pack(fill="both", expand=True, pady=(10, 0))
        
        # Label for car files
        car_files_label = ctk.CTkLabel(
            car_files_frame,
            text=".car Files Found (showing first 10, see CSV for all):",
            font=("Arial", 14, "bold")
        )
        car_files_label.pack(anchor="w", padx=10, pady=(10, 5))
        
        # Create scrollable frame for car files
        self.car_files_scrollable = ctk.CTkScrollableFrame(
            car_files_frame,
            height=200,
            label_text=""
        )
        self.car_files_scrollable.pack(fill="both", expand=True, padx=10, pady=(0, 10))
        
        # Placeholder for car files list
        self.car_files_container = ctk.CTkFrame(self.car_files_scrollable)
        self.car_files_container.pack(fill="both", expand=True)
        
        # CSV status frame
        self.csv_status_frame = ctk.CTkFrame(main_container)
        self.csv_status_frame.pack(fill="x", pady=(10, 5))
        
        self.csv_status_label = ctk.CTkLabel(
            self.csv_status_frame,
            text=f"teams.csv will be saved to: {self.program_dir}\nColumns: FullPath, Filename, RelativePath, Driver, Driver1, Driver2, Description, Team, Number",
            font=("Arial", 11),
            text_color="gray",
            wraplength=800,
            justify="left"
        )
        self.csv_status_label.pack(pady=5, padx=10)
        
        # Action buttons frame
        action_frame = ctk.CTkFrame(main_container)
        action_frame.pack(fill="x", pady=20)
        
        # Copy path button
        copy_button = ctk.CTkButton(
            action_frame,
            text="Copy Path to Clipboard",
            command=self.copy_to_clipboard,
            font=("Arial", 12),
            height=35,
            fg_color="green",
            hover_color="dark green"
        )
        copy_button.pack(side="left", padx=10, pady=10)
        
        # Open folder button
        open_button = ctk.CTkButton(
            action_frame,
            text="Open Folder",
            command=self.open_folder,
            font=("Arial", 12),
            height=35,
            fg_color="blue",
            hover_color="dark blue"
        )
        open_button.pack(side="left", padx=10, pady=10)
        
        # Reset selection button
        reset_button = ctk.CTkButton(
            action_frame,
            text="Reset Selection",
            command=self.reset_selection,
            font=("Arial", 12),
            height=35,
            fg_color="orange",
            hover_color="dark orange"
        )
        reset_button.pack(side="left", padx=10, pady=10)
        
        # Show car files button
        show_cars_button = ctk.CTkButton(
            action_frame,
            text="Show .car Files",
            command=self.show_car_files,
            font=("Arial", 12),
            height=35,
            fg_color="purple",
            hover_color="dark purple"
        )
        show_cars_button.pack(side="left", padx=10, pady=10)
        
        # Generate CSV button
        self.generate_csv_button = ctk.CTkButton(
            action_frame,
            text="Generate teams.csv",
            command=self.generate_csv,
            font=("Arial", 12),
            height=35,
            fg_color="red",
            hover_color="dark red"
        )
        self.generate_csv_button.pack(side="left", padx=10, pady=10)
        self.generate_csv_button.configure(state="disabled")  # Disabled until files are found
        
        # Status bar
        self.status_bar = ctk.CTkLabel(
            main_container,
            text="Ready",
            font=("Arial", 10)
        )
        self.status_bar.pack(fill="x", pady=(10, 0))
    
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
                    self.update_status(f"Selected: {folder_path}")
                    
                    # Find car files immediately using data manager
                    self.car_files = self.data_manager.find_car_files_recursive(folder_path)
                    
                    # Process car files for teams data
                    self.teams_data = self.data_manager.process_car_files_for_teams(folder_path, self.car_files)
                    
                    # Count files in folder for info
                    file_count = len([f for f in os.listdir(folder_path) 
                                    if os.path.isfile(os.path.join(folder_path, f))])
                    
                    # Update car files display
                    self.update_car_files_display()
                    
                    # Enable CSV button if we have car files
                    if self.car_files:
                        self.generate_csv_button.configure(state="normal")
                        self.csv_status_label.configure(
                            text=f"Ready to generate CSV for {len(self.car_files)} car files\nOutput: {os.path.join(self.program_dir, 'teams.csv')}\nColumns: FullPath, Filename, RelativePath, Driver, Driver1, Driver2, Description, Team, Number",
                            text_color="yellow"
                        )
                    else:
                        self.generate_csv_button.configure(state="disabled")
                        self.csv_status_label.configure(
                            text=f"No .car files found for CSV generation\nOutput: {os.path.join(self.program_dir, 'teams.csv')}\nColumns: FullPath, Filename, RelativePath, Driver, Driver1, Driver2, Description, Team, Number",
                            text_color="gray"
                        )
                    
                    messagebox.showinfo(
                        "Folder Selected",
                        f"Successfully selected folder:\n{folder_path}\n\n"
                        f"Contains {file_count} file(s)\n"
                        f"Found {len(self.car_files)} .car file(s) recursively\n"
                        f"Teams data parsed for CSV generation (all fields)\n"
                        f"CSV will be saved to:\n{self.program_dir}"
                    )
                else:
                    messagebox.showwarning(
                        "Invalid Selection",
                        "Please select a folder from within the Teams directory."
                    )
            else:
                self.update_status("Folder selection cancelled")
                
        except Exception as e:
            messagebox.showerror("Error", f"Could not browse folder: {e}")
            self.update_status("Error browsing folder")
    
    def update_car_files_display(self):
        """Update the display of car files"""
        # Clear existing display
        for widget in self.car_files_container.winfo_children():
            widget.destroy()
        
        if not self.car_files:
            no_files_label = ctk.CTkLabel(
                self.car_files_container,
                text="No .car files found in selected folder",
                font=("Arial", 12),
                text_color="gray"
            )
            no_files_label.pack(pady=20)
            return
        
        # Show count
        count_label = ctk.CTkLabel(
            self.car_files_container,
            text=f"Found {len(self.car_files)} .car file(s):",
            font=("Arial", 12, "bold")
        )
        count_label.pack(anchor="w", padx=5, pady=(0, 10))
        
        # Display first 10 car files with parsed info (to avoid UI overload)
        display_limit = min(10, len(self.car_files))
        
        for i in range(display_limit):
            car_file = self.car_files[i]
            team_data = self.teams_data[i]
            
            file_frame = ctk.CTkFrame(self.car_files_container)
            file_frame.pack(fill="x", padx=5, pady=2)
            
            # File info with all extracted data
            driver_info = f"D:{team_data['Driver'] or '(empty)'} | D1:{team_data['Driver1']} | D2:{team_data['Driver2']}"
            additional_info = f"Desc: {team_data['Description'][:30]}{'...' if len(team_data['Description']) > 30 else ''} | Team: {team_data['Team']} | #: {team_data['Number']}"
            file_info = f"{i+1}. {car_file['filename']}\n     {driver_info}\n     {additional_info}"
            
            file_label = ctk.CTkLabel(
                file_frame,
                text=file_info,
                font=("Arial", 10),
                anchor="w",
                justify="left"
            )
            file_label.pack(side="left", padx=10, pady=5, fill="x", expand=True)
            
            # Open file button
            open_file_btn = ctk.CTkButton(
                file_frame,
                text="Open",
                command=lambda f=car_file['full_path']: self.open_car_file(f),
                width=60,
                height=25,
                font=("Arial", 10)
            )
            open_file_btn.pack(side="right", padx=5, pady=5)
        
        # Show message if there are more files
        if len(self.car_files) > display_limit:
            more_files_label = ctk.CTkLabel(
                self.car_files_container,
                text=f"... and {len(self.car_files) - display_limit} more files (see CSV for complete list)",
                font=("Arial", 10, "italic"),
                text_color="gray"
            )
            more_files_label.pack(pady=10)
    
    def generate_csv(self):
        """Generate teams.csv file from parsed car file data"""
        if not self.teams_data:
            messagebox.showwarning("No Data", "No teams data to save. Please select a folder with .car files first.")
            return
        
        try:
            # Use program directory for output
            output_path = os.path.join(self.program_dir, "teams.csv")
            
            # Ask for confirmation if file already exists
            if os.path.exists(output_path):
                response = messagebox.askyesno(
                    "File Exists",
                    f"teams.csv already exists at:\n{output_path}\n\nDo you want to overwrite it?"
                )
                if not response:
                    return
            
            # Save CSV
            success, result, saved_path = self.data_manager.save_teams_csv(self.teams_data, self.program_dir)
            
            if success:
                self.csv_status_label.configure(
                    text=f"teams.csv saved: {result} entries\nLocation: {saved_path}\nColumns: FullPath, Filename, RelativePath, Driver, Driver1, Driver2, Description, Team, Number",
                    text_color="light green"
                )
                self.update_status(f"CSV saved: {result} entries")
                messagebox.showinfo(
                    "Success",
                    f"teams.csv has been successfully created!\n\n"
                    f"Location: {saved_path}\n"
                    f"Entries: {result}\n"
                    f"Columns: FullPath, Filename, RelativePath, Driver, Driver1, Driver2, Description, Team, Number"
                )
            else:
                error_msg = f"Failed to save CSV: {result}"
                self.csv_status_label.configure(
                    text=error_msg,
                    text_color="red"
                )
                self.update_status("CSV save failed")
                messagebox.showerror("Error", error_msg)
                
        except Exception as e:
            error_msg = f"Error generating CSV: {e}"
            self.csv_status_label.configure(text=error_msg, text_color="red")
            self.update_status("CSV generation error")
            messagebox.showerror("Error", error_msg)
    
    def open_car_file(self, file_path):
        """Open the .car file in the default application"""
        try:
            os.startfile(file_path)
            self.update_status(f"Opened: {os.path.basename(file_path)}")
        except Exception as e:
            messagebox.showerror("Error", f"Could not open file: {e}")
    
    def show_car_files(self):
        """Show car files for the current selection"""
        current_folder = self.selected_folder.get()
        
        if not current_folder or current_folder == "No folder selected":
            messagebox.showwarning("No Selection", "Please select a folder first.")
            return
        
        if not os.path.exists(current_folder):
            messagebox.showwarning("Error", "Selected folder does not exist.")
            return
        
        self.car_files = self.data_manager.find_car_files_recursive(current_folder)
        self.teams_data = self.data_manager.process_car_files_for_teams(current_folder, self.car_files)
        self.update_car_files_display()
        
        # Enable/disable CSV button
        if self.car_files:
            self.generate_csv_button.configure(state="normal")
            self.csv_status_label.configure(
                text=f"Ready to generate CSV for {len(self.car_files)} car files\nOutput: {os.path.join(self.program_dir, 'teams.csv')}\nColumns: FullPath, Filename, RelativePath, Driver, Driver1, Driver2, Description, Team, Number",
                text_color="yellow"
            )
        else:
            self.generate_csv_button.configure(state="disabled")
            self.csv_status_label.configure(
                text=f"No .car files found for CSV generation\nOutput: {os.path.join(self.program_dir, 'teams.csv')}\nColumns: FullPath, Filename, RelativePath, Driver, Driver1, Driver2, Description, Team, Number",
                text_color="gray"
            )
        
        if self.car_files:
            messagebox.showinfo(
                "Car Files Found",
                f"Found {len(self.car_files)} .car file(s) in:\n{current_folder}\n\n"
                f"Teams data parsed for CSV generation (all fields).\n"
                f"CSV will be saved to:\n{os.path.join(self.program_dir, 'teams.csv')}"
            )
        else:
            messagebox.showinfo(
                "No Car Files",
                f"No .car files found in:\n{current_folder}"
            )
    
    def copy_to_clipboard(self):
        """Copy selected path to clipboard"""
        path = self.selected_folder.get()
        if path and path != "No folder selected":
            self.clipboard_clear()
            self.clipboard_append(path)
            self.update_status("Path copied to clipboard")
            messagebox.showinfo("Copied", f"Path copied to clipboard:\n{path}")
        else:
            messagebox.showwarning("No Selection", "Please select a folder first.")
    
    def open_folder(self):
        """Open the selected folder in file explorer"""
        path = self.selected_folder.get()
        if path and path != "No folder selected" and os.path.exists(path):
            try:
                os.startfile(path)  # Windows
                self.update_status("Folder opened")
            except:
                try:
                    # Try alternative for other OS (though GTR2 is Windows)
                    import subprocess
                    subprocess.run(['explorer', path])  # Alternative for Windows
                    self.update_status("Folder opened")
                except Exception as e:
                    messagebox.showerror("Error", f"Could not open folder: {e}")
        else:
            messagebox.showwarning("Error", "Selected folder does not exist or no folder selected.")
    
    def reset_selection(self):
        """Reset the folder selection"""
        self.selected_folder.set("No folder selected")
        self.car_files = []
        self.teams_data = []
        self.update_car_files_display()
        self.generate_csv_button.configure(state="disabled")
        self.csv_status_label.configure(
            text=f"teams.csv will be saved to: {self.program_dir}\nColumns: FullPath, Filename, RelativePath, Driver, Driver1, Driver2, Description, Team, Number",
            text_color="gray"
        )
        self.update_status("Selection reset")
        messagebox.showinfo("Reset", "Folder selection has been reset.")
    
    def update_status(self, message):
        """Update status bar message"""
        self.status_bar.configure(text=f"Status: {message}")
    
    def on_closing(self):
        """Handle window closing"""
        self.destroy()
