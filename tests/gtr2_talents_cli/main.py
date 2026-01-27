import os
import re
import csv
from collections import defaultdict
import sys

def main_simple():
    """Simple version without argparse for testing."""
    
    # Use command line arguments or hardcode paths for testing
    if len(sys.argv) >= 4:
        car_path = sys.argv[1]
        rcd_path1 = sys.argv[2]
        rcd_path2 = sys.argv[3]
        output_csv = sys.argv[4] if len(sys.argv) > 4 else "driver_data.csv"
    else:
        # Hardcode paths for testing
        car_path = "/home/aaf/Software/Dev/00_steamonly/GameData/Teams/GT/Chrysler Viper Teams"
        rcd_path1 = "/home/aaf/Software/Dev/00_steamonly/GameData/Teams/GT/Chrysler Viper Teams"
        rcd_path2 = "/home/aaf/Software/Dev/00_steamonly/GameData/Talent"
        output_csv = "test.csv"
    
    print(f"CAR path: {car_path}")
    print(f"RCD path 1: {rcd_path1}")
    print(f"RCD path 2: {rcd_path2}")
    
    # Step 1: Extract drivers from .car files
    drivers = set()
    
    for root, dirs, files in os.walk(car_path):
        for file in files:
            if file.lower().endswith('.car'):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        content = f.read()
                        # Find driver assignments
                        pattern = r'(driver|driver1|driver2)\s*=\s*["\']([^"\']+)["\']'
                        matches = re.findall(pattern, content, re.IGNORECASE)
                        for var_name, driver_name in matches:
                            drivers.add(driver_name.strip())
                except:
                    pass
    
    print(f"Found {len(drivers)} drivers: {sorted(drivers)[:5]}...")
    
    # Step 2: Find .rcd files
    rcd_files = []
    for path in [rcd_path1, rcd_path2]:
        if os.path.exists(path):
            for root, dirs, files in os.walk(path):
                for file in files:
                    if file.lower().endswith('.rcd'):
                        rcd_files.append(os.path.join(root, file))
    
    print(f"Found {len(rcd_files)} .rcd files")
    
    # Step 3: Match .rcd files with drivers by first line
    driver_to_rcd = defaultdict(list)
    
    for rcd_file in rcd_files:
        try:
            with open(rcd_file, 'r', encoding='utf-8', errors='ignore') as f:
                first_line = f.readline().strip()
                if first_line in drivers:
                    driver_to_rcd[first_line].append(rcd_file)
                    print(f"  Match: {first_line} -> {os.path.basename(rcd_file)}")
        except:
            pass
    
    print(f"Matched {len(driver_to_rcd)} drivers to .rcd files")
    
    # Step 4: Extract variables from .rcd files
    driver_data = defaultdict(dict)
    
    for driver, rcd_list in driver_to_rcd.items():
        for rcd_file in rcd_list:
            try:
                with open(rcd_file, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()[1:]  # Skip first line
                    for line in lines:
                        line = line.strip()
                        if not line or line.startswith('{'):
                            continue
                        
                        # Look for variable assignment
                        if '=' in line:
                            # Remove comments
                            if '#' in line:
                                line = line.split('#')[0]
                            
                            parts = line.split('=', 1)
                            if len(parts) == 2:
                                var_name = parts[0].strip()
                                var_value = parts[1].strip()
                                
                                # Remove quotes
                                if (var_value.startswith('"') and var_value.endswith('"')) or \
                                   (var_value.startswith("'") and var_value.endswith("'")):
                                    var_value = var_value[1:-1]
                                
                                driver_data[driver][var_name] = var_value
            except:
                pass
    
    # Step 5: Save to CSV
    if driver_data:
        all_vars = sorted(set().union(*[set(data.keys()) for data in driver_data.values()]))
        
        with open(output_csv, 'w', newline='', encoding='utf-8') as f:
            writer = csv.writer(f)
            writer.writerow(['Driver'] + all_vars)
            
            for driver in sorted(driver_data.keys()):
                row = [driver] + [driver_data[driver].get(var, '') for var in all_vars]
                writer.writerow(row)
        
        print(f"\n✅ Saved data for {len(driver_data)} drivers to {output_csv}")
        print(f"   Variables per driver: {len(all_vars)}")
    else:
        print("\n❌ No data collected")
        print("Check if .rcd files have the driver name as the first line")

if __name__ == "__main__":
    main_simple()
