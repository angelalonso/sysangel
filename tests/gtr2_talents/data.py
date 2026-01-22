import os
import csv
from pathlib import Path

class DataManager:
    """Manages data operations for the application"""
    
    def __init__(self, gtr2_path):
        self.car_files_cache = {}
        self.parsed_car_data = {}
        self.gtr2_path = gtr2_path
        self.talent_path = os.path.join(gtr2_path, "GameData", "Talent") if gtr2_path else None
    
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
                        'directory': root,
                        'parent_directory': os.path.dirname(root)
                    })
        
        # Sort by filename
        car_files.sort(key=lambda x: x['filename'].lower())
        
        # Cache the results
        self.car_files_cache[folder_path] = car_files
        
        return car_files
    
    def parse_car_file(self, file_path):
        """Parse a .car file and extract all required values (ignore comments)"""
        car_data = {
            'Driver': '',
            'Driver1': '',
            'Driver2': '',
            'Description': '',
            'Team': '',
            'car_number': ''
        }
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as file:
                lines = file.readlines()
                
                for line in lines:
                    line = line.strip()
                    
                    # Remove comments (everything after //)
                    if '//' in line:
                        line = line.split('//')[0].strip()
                    
                    # Skip empty lines after comment removal
                    if not line:
                        continue
                    
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
                        car_data['car_number'] = line.split('=', 1)[1].strip()
                    
                    # Check if we found all required fields
                    found_all = all(value != '' for value in car_data.values())
                    if found_all:
                        break
                        
        except Exception as e:
            print(f"Error parsing file {file_path}: {e}")
            # Return empty values if file can't be read
            return car_data
        
        return car_data
    
    def normalize_driver_name(self, driver_name):
        """Normalize driver name for comparison - remove quotes and extra spaces"""
        if not driver_name:
            return ''
        
        # Remove any quotes (single or double) from the beginning and end
        driver_name = driver_name.strip()
        if (driver_name.startswith('"') and driver_name.endswith('"')) or \
           (driver_name.startswith("'") and driver_name.endswith("'")):
            driver_name = driver_name[1:-1].strip()
        
        # Convert to lowercase for case-insensitive comparison
        return driver_name.lower()
    
    def find_rcd_file_for_driver(self, driver_name, car_file_directory, car_file_parent_dir):
        """Find .rcd file for a driver by searching in multiple locations"""
        if not driver_name:
            return ''
        
        # Normalize the driver name we're searching for
        normalized_driver_name = self.normalize_driver_name(driver_name)
        
        # List of directories to search (in order of priority)
        search_dirs = []
        
        # 1. Current directory where .car file is located
        if car_file_directory:
            search_dirs.append(car_file_directory)
        
        # 2. Parent directory of .car file location
        if car_file_parent_dir:
            search_dirs.append(car_file_parent_dir)
        
        # 3. GameData/Talent directory
        if self.talent_path and os.path.exists(self.talent_path):
            search_dirs.append(self.talent_path)
        
        # Search for .rcd files in all directories
        for search_dir in search_dirs:
            if not os.path.exists(search_dir):
                continue
                
            try:
                # Search recursively in the directory
                for root, dirs, files in os.walk(search_dir):
                    for file in files:
                        if file.lower().endswith('.rcd'):
                            rcd_path = os.path.join(root, file)
                            
                            # Check if first line matches driver name
                            try:
                                with open(rcd_path, 'r', encoding='utf-8', errors='ignore') as rcd_file:
                                    first_line = rcd_file.readline().strip()
                                    
                                    # Remove comments from first line
                                    if '//' in first_line:
                                        first_line = first_line.split('//')[0].strip()
                                    
                                    # Normalize the name from the RCD file
                                    normalized_rcd_name = self.normalize_driver_name(first_line)
                                    
                                    # Check for exact match after normalization
                                    if normalized_rcd_name == normalized_driver_name:
                                        return rcd_path
                                    
                                    # Also check if driver name is contained in the first line
                                    # (useful if there are extra characters or formatting)
                                    if normalized_driver_name in normalized_rcd_name:
                                        return rcd_path
                                    
                                    # Check the other way around too
                                    if normalized_rcd_name in normalized_driver_name:
                                        return rcd_path
                            except:
                                continue  # Skip if can't read the file
            except:
                continue  # Skip if can't walk the directory
        
        return ''  # Return empty string if not found
    
    def parse_rcd_file(self, rcd_path):
        """Parse an RCD file and extract all key-value pairs (ignore comments)"""
        rcd_data = {}
        
        if not rcd_path or not os.path.exists(rcd_path):
            return rcd_data
        
        try:
            with open(rcd_path, 'r', encoding='utf-8', errors='ignore') as file:
                lines = file.readlines()
                
                for line in lines:
                    line = line.strip()
                    
                    # Remove comments (everything after //)
                    if '//' in line:
                        line = line.split('//')[0].strip()
                    
                    # Skip empty lines after comment removal
                    if not line:
                        continue
                    
                    if '=' in line:
                        key, value = line.split('=', 1)
                        key = key.strip()
                        value = value.strip()
                        
                        # Remove quotes from value if present
                        if (value.startswith('"') and value.endswith('"')) or \
                           (value.startswith("'") and value.endswith("'")):
                            value = value[1:-1]
                        
                        rcd_data[key] = value
            
            # Ensure we have at least the driver name (first line)
            if not rcd_data and lines:
                first_line = lines[0].strip()
                if '//' in first_line:
                    first_line = first_line.split('//')[0].strip()
                rcd_data['DriverName'] = self.normalize_driver_name(first_line)
                
        except Exception as e:
            print(f"Error parsing RCD file {rcd_path}: {e}")
        
        return rcd_data
    
    def process_car_files_for_talents(self, folder_path, car_files):
        """Process all car files and create talents data table - one entry per driver"""
        talents_data = []
        
        for car_file in car_files:
            full_path = car_file['full_path']
            
            # Parse the car file
            car_data = self.parse_car_file(full_path)
            
            # Get all unique drivers from this car file
            drivers = []
            driver_positions = {}
            
            # Add Driver1 if exists and not empty
            if car_data['Driver1']:
                drivers.append(car_data['Driver1'])
                driver_positions[car_data['Driver1']] = 'Driver1'
            
            # Add Driver2 if exists and not empty
            if car_data['Driver2']:
                drivers.append(car_data['Driver2'])
                driver_positions[car_data['Driver2']] = 'Driver2'
            
            # Add Driver if exists, not empty, and not same as Driver1 or Driver2
            if car_data['Driver'] and car_data['Driver'] not in [car_data['Driver1'], car_data['Driver2']]:
                drivers.append(car_data['Driver'])
                driver_positions[car_data['Driver']] = 'Driver'
            
            # Create an entry for each unique driver
            for driver in set(drivers):  # Use set to avoid duplicates
                # Find RCD file for this driver
                rcd_path = self.find_rcd_file_for_driver(
                    driver, 
                    car_file['directory'], 
                    car_file['parent_directory']
                )
                
                # Parse RCD file if found
                rcd_data = {}
                if rcd_path:
                    rcd_data = self.parse_rcd_file(rcd_path)
                
                # Create talent entry with RCD data
                talent_entry = {
                    'Driver': driver,
                    'Position': driver_positions[driver],
                    'RCDPath': rcd_path,
                    'RCDExists': bool(rcd_path),
                    'CARFile': car_file['filename'],
                    'CARFullPath': full_path,
                    'CARRelativePath': car_file['relative_path'],
                    'Description': car_data['Description'],
                    'Team': car_data['Team'],
                    'car_number': car_data['car_number'],
                    'Directory': car_file['directory']
                }
                
                # Add all RCD variables to the entry (WITH RCD_ PREFIX for CSV)
                for key, value in rcd_data.items():
                    # Clean up key names for CSV (no spaces, etc.)
                    clean_key = key.replace(' ', '_').replace('-', '_').replace('(', '').replace(')', '')
                    talent_entry[f'RCD_{clean_key}'] = value
                
                talents_data.append(talent_entry)
        
        # Sort by driver name
        talents_data.sort(key=lambda x: x['Driver'].lower())
        
        return talents_data
    
    def save_teams_csv(self, talents_data, output_dir):
        """Save teams data to a CSV file in the specified directory - ONE ENTRY PER DRIVER WITH RCD DATA"""
        try:
            # Create output path in the program directory - CHANGED TO teams.csv
            output_path = os.path.join(output_dir, "teams.csv")
            
            # Collect all possible columns (including dynamic RCD columns)
            fieldnames = set()
            for entry in talents_data:
                fieldnames.update(entry.keys())
            
            # Sort fieldnames for consistent output
            fieldnames = sorted(list(fieldnames))
            
            # Ensure key columns come first in logical order
            key_columns = [
                'Driver', 
                'RCDPath', 
                'RCDExists',
                'CARFile', 
                'CARFullPath', 
                'CARRelativePath',
                'Team', 
                'car_number',
                'Description',
                'Position',
                'Directory'
            ]
            
            # Reorder to put key columns first
            ordered_fieldnames = []
            for col in key_columns:
                if col in fieldnames:
                    ordered_fieldnames.append(col)
                    fieldnames.remove(col)
            
            # Add RCD_ columns (sorted alphabetically)
            rcd_columns = sorted([col for col in fieldnames if col.startswith('RCD_')])
            ordered_fieldnames.extend(rcd_columns)
            
            # Add any remaining columns
            other_columns = sorted([col for col in fieldnames if not col.startswith('RCD_')])
            ordered_fieldnames.extend(other_columns)
            
            with open(output_path, 'w', newline='', encoding='utf-8') as csvfile:
                writer = csv.DictWriter(csvfile, fieldnames=ordered_fieldnames)
                
                # Write header
                writer.writeheader()
                
                # Write data rows - ONE ENTRY PER DRIVER WITH ALL DATA
                for talent in talents_data:
                    # Ensure all columns exist in this row
                    row_data = {}
                    for col in ordered_fieldnames:
                        row_data[col] = talent.get(col, '')
                    writer.writerow(row_data)
            
            return True, len(talents_data), output_path
            
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
