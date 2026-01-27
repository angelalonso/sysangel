import os
import csv
import shutil
from datetime import datetime
from pathlib import Path

class TalentManager:
    """Manages RCD file operations, backups, and generation"""
    
    def __init__(self, program_dir):
        self.program_dir = program_dir
        self.backup_dir = os.path.join(program_dir, "backups")
        self.original_talents_path = os.path.join(self.backup_dir, "original_talents.csv")
        
        # Create backup directory if it doesn't exist
        if not os.path.exists(self.backup_dir):
            os.makedirs(self.backup_dir)
    
    def backup_original_rcd(self, rcd_path, driver_name):
        """Create a backup of the original RCD file"""
        if not rcd_path or not os.path.exists(rcd_path):
            return False
        
        try:
            # Create timestamp for backup
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            
            # Create backup filename
            driver_safe = driver_name.replace(' ', '_').replace('/', '_').replace('\\', '_')
            backup_filename = f"{driver_safe}_{timestamp}.rcd"
            backup_path = os.path.join(self.backup_dir, backup_filename)
            
            # Copy the file
            shutil.copy2(rcd_path, backup_path)
            
            print(f"Backed up RCD file: {backup_path}")
            return True
            
        except Exception as e:
            print(f"Error backing up RCD file: {e}")
            return False
    
    def save_original_talent_to_csv(self, talent_entry):
        """Save original talent data to backup CSV file"""
        try:
            # Prepare the data for CSV
            csv_data = talent_entry.copy()
            
            # Add timestamp
            csv_data['BackupTimestamp'] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            
            # Check if file exists
            file_exists = os.path.exists(self.original_talents_path)
            
            # Write to CSV
            with open(self.original_talents_path, 'a' if file_exists else 'w', newline='', encoding='utf-8') as csvfile:
                # Get all possible keys
                fieldnames = set(csv_data.keys())
                
                # Remove BackupTimestamp and add it at the end
                fieldnames.discard('BackupTimestamp')
                fieldnames = sorted(list(fieldnames))
                fieldnames.append('BackupTimestamp')
                
                writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                
                # Write header if file doesn't exist
                if not file_exists:
                    writer.writeheader()
                
                # Check if driver already exists in CSV
                driver_exists = False
                if file_exists:
                    with open(self.original_talents_path, 'r', encoding='utf-8') as f:
                        reader = csv.DictReader(f)
                        for row in reader:
                            if row.get('Driver') == csv_data.get('Driver') and row.get('RCDPath') == csv_data.get('RCDPath'):
                                driver_exists = True
                                break
                
                # Only write if driver doesn't exist
                if not driver_exists:
                    writer.writerow(csv_data)
                    print(f"Saved original talent data for {csv_data.get('Driver')} to backup CSV")
                    return True
                else:
                    print(f"Driver {csv_data.get('Driver')} already exists in backup CSV")
                    return False
                    
        except Exception as e:
            print(f"Error saving to backup CSV: {e}")
            return False
    
    def generate_rcd_content(self, talent_data):
        """Generate RCD file content from talent data using template"""
        # Extract RCD values (remove RCD_ prefix)
        rcd_values = {}
        for key, value in talent_data.items():
            if key.startswith('RCD_'):
                rcd_key = key[4:]  # Remove RCD_ prefix
                rcd_values[rcd_key] = value
        
        # Get driver name (use the actual driver name from the talent data)
        driver_name = talent_data.get('Driver', 'Unknown Driver')
        
        # Template for RCD file
        template = f"""{driver_name}
{{
 //Driver Info
 Abbreviation={rcd_values.get('Abbreviation', '')}
 Nationality={rcd_values.get('Nationality', '')}
 NatAbbrev={rcd_values.get('NatAbbrev', '')}
 
 //Driver Stats
 StartsDry={rcd_values.get('StartsDry', '0.0')}                      //Average number of drivers passed during start (-4 - 4)
 StartsWet={rcd_values.get('StartsWet', '0.0')}           
 StartStalls={rcd_values.get('StartStalls', '0.0')}                       //% of starts where driver stalled
 QualifyingAbility={rcd_values.get('QualifyingAbility', '0.0')}          //Average qualifying position NOTE: keep GT between 1 -15
 RaceAbility={rcd_values.get('RaceAbility', '0.0')}                //Range 0 - 6.2 (0 is best)
 Consistency={rcd_values.get('Consistency', '0.0')}
 RainAbility={rcd_values.get('RainAbility', '0.0')}                  //Range 0 - 6.2 (0 is best)
 Passing={rcd_values.get('Passing', '0.0')}                     //% of times driver completed a successfull pass, not including pit stops or lapped traffic
 Crash={rcd_values.get('Crash', '0.0')}                          //% of times driver crashed
 Recovery={rcd_values.get('Recovery', '0.0')}                      //% of times driver continued after a crash
 CompletedLaps%={rcd_values.get('CompletedLaps%', '0.0')}
 Script={rcd_values.get('Script', 'default.scp')}
 TrackAggression={rcd_values.get('TrackAggression', '0.0')}


 // Increase attempted low-speed cornering by adding a minimum onto calculated speed.
 // Reduce attempted high-speed cornering by multiplying speed by a number less than 1.0.
 // <adjusted speed> = CorneringAdd + (CorneringMult * <original speed>)
  CorneringAdd={rcd_values.get('CorneringAdd', '0.0')}
 CorneringMult={rcd_values.get('CorneringMult', '1.0')}

//AI Throttle Control - how good they are at their own traction control upon throttle application
TCGripThreshold={rcd_values.get('TCGripThreshold', '0.0')}   // Range: 0.0-1.0
TCThrottleFract={rcd_values.get('TCThrottleFract', '0.0')}   // Range: 0.0-???
TCResponse={rcd_values.get('TCResponse', '0.0')}        // Range: 0.0-???

//AI skill mistake variables
MinRacingSkill = {rcd_values.get('MinRacingSkill', '0.0')}
Composure = {rcd_values.get('Composure', '0.0')}

//AI ColdBrain variables
RaceColdBrainMin={rcd_values.get('RaceColdBrainMin', '0.0')}
RaceColdBrainTime={rcd_values.get('RaceColdBrainTime', '0')}
QualColdBrainMin={rcd_values.get('QualColdBrainMin', '0.0')} 
QualColdBrainTime={rcd_values.get('QualColdBrainTime', '0')}

}}
"""
        return template
    
    def update_rcd_file(self, talent_data, original_talent_data=None):
        """Update RCD file with new values, creating backups first"""
        rcd_path = talent_data.get('RCDPath', '')
        driver_name = talent_data.get('Driver', '')
        
        if not rcd_path or not os.path.exists(rcd_path):
            print(f"RCD file not found: {rcd_path}")
            return False
        
        try:
            # 1. Backup original RCD file
            self.backup_original_rcd(rcd_path, driver_name)
            
            # 2. Save original talent data to backup CSV (if provided)
            if original_talent_data:
                self.save_original_talent_to_csv(original_talent_data)
            
            # 3. Generate new RCD content
            rcd_content = self.generate_rcd_content(talent_data)
            
            # 4. Write to RCD file
            with open(rcd_path, 'w', encoding='utf-8') as f:
                f.write(rcd_content)
            
            print(f"Updated RCD file: {rcd_path}")
            return True
            
        except Exception as e:
            print(f"Error updating RCD file: {e}")
            return False
    
    def update_multiple_rcd_files(self, updated_talents, original_talents):
        """Update multiple RCD files"""
        results = []
        
        for updated_talent, original_talent in zip(updated_talents, original_talents):
            success = self.update_rcd_file(updated_talent, original_talent)
            results.append({
                'driver': updated_talent.get('Driver', 'Unknown'),
                'rcd_path': updated_talent.get('RCDPath', ''),
                'success': success
            })
        
        return results
