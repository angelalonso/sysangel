import os
import csv
from pathlib import Path

class DataManager:
    """Manages data operations for the application"""
    
    def __init__(self):
        self.car_files_cache = {}
        self.parsed_car_data = {}
    
    def find_car_files_recursive(self, folder_path):
        """Find all .car files recursively in the given folder"""
        car_files = []
        
        # Check cache first
        if folder_path in self.car_files_cache:
            return self.car_files_cache[folder_path]
        
        for root, dirs, files in os.walk(folder_path):
            for file in files:
                if file.lower().endswith('.car'):
                    full_path = os.path.join(root, file)
                    # Get relative path from the selected folder
                    rel_path = os.path.relpath(full_path, folder_path)
                    car_files.append({
                        'full_path': full_path,
                        'relative_path': rel_path,
                        'filename': file,
                        'directory': root
                    })
        
        # Sort by filename
        car_files.sort(key=lambda x: x['filename'].lower())
        
        # Cache the results
        self.car_files_cache[folder_path] = car_files
        
        return car_files
    
    def parse_car_file(self, file_path):
        """Parse a .car file and extract all required values"""
        car_data = {
            'Driver': '',
            'Driver1': '',
            'Driver2': '',
            'Description': '',
            'Team': '',
            'Number': ''
        }
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
                lines = file.readlines()
                
                for line in lines:
                    line = line.strip()
                    
                    # Look for all required entries
                    if line.startswith('Driver='):
                        car_data['Driver'] = line.split('=', 1)[1].strip()
                    elif line.startswith('Driver1='):
                        car_data['Driver1'] = line.split('=', 1)[1].strip()
                    elif line.startswith('Driver2='):
                        car_data['Driver2'] = line.split('=', 1)[1].strip()
                    elif line.startswith('Description='):
                        car_data['Description'] = line.split('=', 1)[1].strip()
                    elif line.startswith('Team='):
                        car_data['Team'] = line.split('=', 1)[1].strip()
                    elif line.startswith('Number='):
                        car_data['Number'] = line.split('=', 1)[1].strip()
                    
                    # Check if we found all required fields
                    found_all = all(value != '' for value in car_data.values())
                    if found_all:
                        break
                        
        except Exception as e:
            print(f"Error parsing file {file_path}: {e}")
            # Return empty values if file can't be read
            return car_data
        
        return car_data
    
    def process_car_files_for_teams(self, folder_path, car_files):
        """Process all car files and create teams data table"""
        teams_data = []
        
        for car_file in car_files:
            full_path = car_file['full_path']
            
            # Parse the car file
            car_data = self.parse_car_file(full_path)
            
            # Apply logic: If Driver is the same as Driver1 or Driver2, leave Driver empty
            driver_value = car_data['Driver']
            if driver_value == car_data['Driver1'] or driver_value == car_data['Driver2']:
                driver_value = ''
            
            # Add to teams data
            team_entry = {
                'FullPath': full_path,
                'Filename': car_file['filename'],
                'RelativePath': car_file['relative_path'],
                'Driver': driver_value,
                'Driver1': car_data['Driver1'],
                'Driver2': car_data['Driver2'],
                'Description': car_data['Description'],
                'Team': car_data['Team'],
                'Number': car_data['Number']
            }
            
            teams_data.append(team_entry)
        
        return teams_data
    
    def save_teams_csv(self, teams_data, output_dir):
        """Save teams data to a CSV file in the specified directory"""
        try:
            # Create output path in the program directory
            output_path = os.path.join(output_dir, "teams.csv")
            
            # Define CSV fieldnames with all required columns
            fieldnames = [
                'FullPath',
                'Filename', 
                'RelativePath',
                'Driver',
                'Driver1',
                'Driver2',
                'Description',
                'Team',
                'Number'
            ]
            
            with open(output_path, 'w', newline='', encoding='utf-8') as csvfile:
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                
                # Write header
                writer.writeheader()
                
                # Write data rows
                writer.writerows(teams_data)
            
            return True, len(teams_data), output_path
            
        except Exception as e:
            return False, str(e), None
    
    def clear_cache(self):
        """Clear the car files cache"""
        self.car_files_cache.clear()
        self.parsed_car_data.clear()
    
    def get_car_file_info(self, file_path):
        """Get detailed information about a .car file"""
        if not os.path.exists(file_path):
            return None
        
        file_info = {
            'path': file_path,
            'size': os.path.getsize(file_path),
            'modified': os.path.getmtime(file_path),
            'created': os.path.getctime(file_path),
        }
        
        return file_info
