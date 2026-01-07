#!/usr/bin/env python3
"""
Script to find duplicate files between two directories that can be safely deleted from the first path.
Supports interruption recovery with checkpoint files.
"""

import os
import sys
import hashlib
import json
import argparse
from pathlib import Path
from typing import Dict, List, Set, Tuple, Optional
import time

class DuplicateFinder:
    def __init__(self, path1: str, path2: str, checkpoint_file: str = "checkpoint.json"):
        self.path1 = Path(path1).resolve()
        self.path2 = Path(path2).resolve()
        self.checkpoint_file = checkpoint_file
        
        # Load or initialize checkpoint
        self.checkpoint = self._load_checkpoint()
        
        # Results storage
        self.duplicates_stage1 = []
        self.duplicates_stage2 = []
        
    def _load_checkpoint(self) -> Dict:
        """Load checkpoint data from file if it exists."""
        if os.path.exists(self.checkpoint_file):
            try:
                with open(self.checkpoint_file, 'r') as f:
                    return json.load(f)
            except (json.JSONDecodeError, IOError):
                print(f"Warning: Could not load checkpoint from {self.checkpoint_file}")
                return self._create_default_checkpoint()
        return self._create_default_checkpoint()
    
    def _create_default_checkpoint(self) -> Dict:
        """Create a default checkpoint structure."""
        return {
            "stage": 1,
            "last_checked_index": 0,
            "last_checked_file": "",
            "files1": [],
            "files2": [],
            "processed_files1": [],
            "processed_files2": [],
            "files1_sizes": {},
            "files2_sizes": {},
            "duplicates_stage1": [],
            "duplicates_stage2": []
        }
    
    def _save_checkpoint(self):
        """Save current state to checkpoint file."""
        self.checkpoint.update({
            "stage": self.current_stage,
            "last_checked_index": self.last_checked_index,
            "last_checked_file": self.last_checked_file,
            "files1": [str(f) for f in self.files1],
            "files2": [str(f) for f in self.files2],
            "processed_files1": [str(f) for f in self.processed_files1],
            "processed_files2": [str(f) for f in self.processed_files2],
            "files1_sizes": self.files1_sizes,
            "files2_sizes": self.files2_sizes,
            "duplicates_stage1": self.duplicates_stage1,
            "duplicates_stage2": self.duplicates_stage2
        })
        
        try:
            with open(self.checkpoint_file, 'w') as f:
                json.dump(self.checkpoint, f, indent=2)
        except IOError as e:
            print(f"Warning: Could not save checkpoint: {e}")
    
    def _calculate_checksum(self, filepath: Path, buffer_size: int = 65536) -> str:
        """Calculate MD5 checksum of a file."""
        md5 = hashlib.md5()
        try:
            with open(filepath, 'rb') as f:
                while chunk := f.read(buffer_size):
                    md5.update(chunk)
            return md5.hexdigest()
        except (IOError, OSError) as e:
            print(f"Error reading {filepath}: {e}")
            return ""
    
    def _get_all_files(self, directory: Path) -> List[Path]:
        """Recursively get all files from a directory."""
        files = []
        try:
            for item in directory.rglob('*'):
                if item.is_file():
                    files.append(item)
        except (PermissionError, OSError) as e:
            print(f"Warning: Could not access {directory}: {e}")
        return files
    
    def _save_file_lists(self):
        """Save file lists to text files."""
        try:
            with open('files_path1.txt', 'w') as f:
                for filepath in self.files1:
                    f.write(f"{filepath}\n")
            print(f"Saved {len(self.files1)} files from path1 to files_path1.txt")
            
            with open('files_path2.txt', 'w') as f:
                for filepath in self.files2:
                    f.write(f"{filepath}\n")
            print(f"Saved {len(self.files2)} files from path2 to files_path2.txt")
        except IOError as e:
            print(f"Error saving file lists: {e}")
    
    def _save_results_pt1(self):
        """Save duplicate results  of stage 1 to files."""
        try:
            # Save stage 1 results (exact name matches)
            with open('duplicates_stage1_tmp.txt', 'w') as f:
                for dup in self.duplicates_stage1:
                    f.write(f"Path1: {dup['path1']}\n")
                    f.write(f"Path2: {dup['path2']}\n")
                    f.write(f"Size: {dup['size']} bytes\n")
                    f.write(f"Checksum: {dup['checksum']}\n")
                    f.write("-" * 80 + "\n")
            print(f"Saved {len(self.duplicates_stage1)} stage 1 duplicates to duplicates_stage1.txt")
                    
        except IOError as e:
            print(f"Error saving results: {e}")
    
    def _save_results_pt2(self):
        """Save duplicate results to files."""
        try:
            # Save stage 2 results (size matches)
            with open('duplicates_stage2_tmp.txt', 'w') as f:
                for dup in self.duplicates_stage2:
                    f.write(f"Path1: {dup['path1']}\n")
                    f.write(f"Path2: {dup['path2']}\n")
                    f.write(f"Size: {dup['size']} bytes\n")
                    f.write(f"Checksum: {dup['checksum']}\n")
                    f.write("-" * 80 + "\n")
            print(f"Saved {len(self.duplicates_stage2)} stage 2 duplicates to duplicates_stage2.txt")
                    
        except IOError as e:
            print(f"Error saving results: {e}")
    
    def _save_results(self):
        """Save duplicate results to files."""
        try:
            # Save stage 1 results (exact name matches)
            with open('duplicates_stage1.txt', 'w') as f:
                for dup in self.duplicates_stage1:
                    f.write(f"Path1: {dup['path1']}\n")
                    f.write(f"Path2: {dup['path2']}\n")
                    f.write(f"Size: {dup['size']} bytes\n")
                    f.write(f"Checksum: {dup['checksum']}\n")
                    f.write("-" * 80 + "\n")
            print(f"Saved {len(self.duplicates_stage1)} stage 1 duplicates to duplicates_stage1.txt")
            
            # Save stage 2 results (size matches)
            with open('duplicates_stage2.txt', 'w') as f:
                for dup in self.duplicates_stage2:
                    f.write(f"Path1: {dup['path1']}\n")
                    f.write(f"Path2: {dup['path2']}\n")
                    f.write(f"Size: {dup['size']} bytes\n")
                    f.write(f"Checksum: {dup['checksum']}\n")
                    f.write("-" * 80 + "\n")
            print(f"Saved {len(self.duplicates_stage2)} stage 2 duplicates to duplicates_stage2.txt")
            
            # Save summary
            with open('duplicates_summary.txt', 'w') as f:
                f.write("DUPLICATE FILES SUMMARY\n")
                f.write("=" * 80 + "\n\n")
                f.write(f"Total duplicates found (stage 1): {len(self.duplicates_stage1)}\n")
                f.write(f"Total duplicates found (stage 2): {len(self.duplicates_stage2)}\n")
                f.write(f"Total unique duplicates: {len(self.duplicates_stage1) + len(self.duplicates_stage2)}\n\n")
                
                f.write("Stage 1 duplicates (same filename):\n")
                for i, dup in enumerate(self.duplicates_stage1, 1):
                    f.write(f"{i}. {os.path.basename(dup['path1'])}\n")
                    f.write(f"   From: {dup['path1']}\n")
                    f.write(f"   Duplicate at: {dup['path2']}\n")
                
                f.write("\nStage 2 duplicates (same size, different names):\n")
                for i, dup in enumerate(self.duplicates_stage2, 1):
                    f.write(f"{i}. {os.path.basename(dup['path1'])} (size: {dup['size']:,} bytes)\n")
                    f.write(f"   From: {dup['path1']}\n")
                    f.write(f"   Duplicate at: {dup['path2']}\n")
                    
        except IOError as e:
            print(f"Error saving results: {e}")
    
    def run(self):
        """Main execution method with interruption recovery."""
        print(f"Starting duplicate finder:")
        print(f"  Path 1: {self.path1}")
        print(f"  Path 2: {self.path2}")
        print(f"  Checkpoint file: {self.checkpoint_file}")
        print()
        
        # Restore or create file lists
        if self.checkpoint.get('files1') and self.checkpoint.get('files2'):
            print("Loading file lists from checkpoint...")
            self.files1 = [Path(f) for f in self.checkpoint['files1']]
            self.files2 = [Path(f) for f in self.checkpoint['files2']]
            self.files1_sizes = self.checkpoint.get('files1_sizes', {})
            self.files2_sizes = self.checkpoint.get('files2_sizes', {})
            self.processed_files1 = [Path(f) for f in self.checkpoint['processed_files1']]
            self.processed_files2 = [Path(f) for f in self.checkpoint['processed_files2']]
            self.duplicates_stage1 = self.checkpoint.get('duplicates_stage1', [])
            self.duplicates_stage2 = self.checkpoint.get('duplicates_stage2', [])
        else:
            print("Building file lists...")
            self.files1 = self._get_all_files(self.path1)
            self.files2 = self._get_all_files(self.path2)
            self.files1_sizes = {}
            self.files2_sizes = {}
            self.processed_files1 = []
            self.processed_files2 = []
            self._save_file_lists()
        
        print(f"Found {len(self.files1)} files in path1")
        print(f"Found {len(self.files2)} files in path2")
        print()
        
        # Determine starting point
        self.current_stage = self.checkpoint.get('stage', 1)
        self.last_checked_index = self.checkpoint.get('last_checked_index', 0)
        self.last_checked_file = self.checkpoint.get('last_checked_file', '')
        
        if self.current_stage == 1:
            self._run_stage1()
            self._save_results_pt1()
        
        if self.current_stage == 2 or (self.current_stage == 1 and self.last_checked_index >= len(self.files1)):
            self.current_stage = 2
            self._run_stage2()
            self._save_results_pt2()
        
        # Save final results
        self._save_results()
        print("\nProcessing complete!")
        
        # Clean up checkpoint
        if os.path.exists(self.checkpoint_file):
            os.remove(self.checkpoint_file)
            print(f"Checkpoint file {self.checkpoint_file} removed")
    
    def _run_stage1(self):
        """Stage 1: Find files with same name, size, and checksum."""
        print("=" * 80)
        print("STAGE 1: Finding files with same name")
        print("=" * 80)
        
        # Create a dictionary of files in path2 by name for faster lookup
        files2_by_name = {}
        for f2 in self.files2:
            files2_by_name[f2.name] = files2_by_name.get(f2.name, []) + [f2]
        
        # Process files from path1
        start_idx = self.last_checked_index
        for i in range(start_idx, len(self.files1)):
            file1 = self.files1[i]
            self.last_checked_index = i
            self.last_checked_file = str(file1)
            
            # Get file1 size
            try:
                size1 = file1.stat().st_size
                self.files1_sizes[str(file1)] = size1
            except (OSError, IOError) as e:
                print(f"Skipping {file1}: {e}")
                continue
            
            # Check if there's a file with same name in path2
            if file1.name in files2_by_name:
                for file2 in files2_by_name[file1.name]:
                    try:
                        size2 = file2.stat().st_size
                        self.files2_sizes[str(file2)] = size2
                        
                        # Compare sizes first (faster)
                        if size1 == size2:
                            # Calculate checksums if sizes match
                            checksum1 = self._calculate_checksum(file1)
                            if not checksum1:
                                continue
                                
                            checksum2 = self._calculate_checksum(file2)
                            if not checksum2:
                                continue
                            
                            if checksum1 == checksum2:
                                duplicate = {
                                    'path1': str(file1),
                                    'path2': str(file2),
                                    'size': size1,
                                    'checksum': checksum1
                                }
                                self.duplicates_stage1.append(duplicate)
                                print(f"✓ Duplicate found: {file1.name}")
                                print(f"  {file1}")
                                print(f"  {file2}")
                    
                    except (OSError, IOError) as e:
                        print(f"Skipping {file2}: {e}")
                        continue
            
            # Add to processed list
            self.processed_files1.append(file1)
            
            # Save checkpoint every 100 files
            if i % 100 == 0:
                self._save_checkpoint()
                print(f"Progress: {i+1}/{len(self.files1)} files checked")
        
        print(f"\nStage 1 complete: Found {len(self.duplicates_stage1)} duplicates")
        
        # Update for stage 2
        self.current_stage = 2
        self.last_checked_index = 0
        self.last_checked_file = ""
        self._save_checkpoint()
    
    def _run_stage2(self):
        """Stage 2: Find files with same size (and checksum) but different names."""
        print("\n" + "=" * 80)
        print("STAGE 2: Finding files with same size (different names)")
        print("=" * 80)
        
        # Build size index for path2 files
        print("Building size index for path2 files...")
        size_index = {}
        for file2 in self.files2:
            if str(file2) not in self.files2_sizes:
                try:
                    size = file2.stat().st_size
                    self.files2_sizes[str(file2)] = size
                except (OSError, IOError):
                    continue
            
            size = self.files2_sizes[str(file2)]
            size_index.setdefault(size, []).append(file2)
        
        # Process files from path1 that weren't already matched in stage1
        # (Exclude files that were already identified as duplicates in stage1)
        stage1_path1_files = {dup['path1'] for dup in self.duplicates_stage1}
        files_to_process = [f for f in self.files1 if str(f) not in stage1_path1_files]
        
        start_idx = self.last_checked_index
        for i in range(start_idx, len(files_to_process)):
            file1 = files_to_process[i]
            self.last_checked_index = i
            self.last_checked_file = str(file1)
            
            # Get file1 size
            try:
                if str(file1) in self.files1_sizes:
                    size1 = self.files1_sizes[str(file1)]
                else:
                    size1 = file1.stat().st_size
                    self.files1_sizes[str(file1)] = size1
            except (OSError, IOError) as e:
                print(f"Skipping {file1}: {e}")
                continue
            
            # Check if there are files in path2 with same size
            if size1 in size_index:
                # Calculate checksum for file1
                checksum1 = self._calculate_checksum(file1)
                if not checksum1:
                    continue
                
                # Check each file in path2 with same size
                for file2 in size_index[size1]:
                    # Skip if file2 was already matched in stage1
                    if any(dup['path2'] == str(file2) for dup in self.duplicates_stage1):
                        continue
                    
                    # Calculate checksum for file2 if needed
                    checksum2 = self._calculate_checksum(file2)
                    if not checksum2:
                        continue
                    
                    if checksum1 == checksum2:
                        duplicate = {
                            'path1': str(file1),
                            'path2': str(file2),
                            'size': size1,
                            'checksum': checksum1
                        }
                        self.duplicates_stage2.append(duplicate)
                        print(f"✓ Duplicate found (different name):")
                        print(f"  {file1.name} -> {file2.name}")
                        print(f"  {file1}")
                        print(f"  {file2}")
                        # Remove file2 from index to avoid matching it again
                        size_index[size1].remove(file2)
                        break
            
            # Add to processed list
            self.processed_files2.append(file1)
            
            # Save checkpoint every 100 files
            if i % 100 == 0:
                self._save_checkpoint()
                print(f"Progress: {i+1}/{len(files_to_process)} files checked")
        
        print(f"\nStage 2 complete: Found {len(self.duplicates_stage2)} duplicates")

def main():
    parser = argparse.ArgumentParser(
        description="Find duplicate files between two directories that can be safely deleted from the first path.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s /path/to/dir1 /path/to/dir2
  %(prog)s /path/to/dir1 /path/to/dir2 --checkpoint my_checkpoint.json
        """
    )
    
    parser.add_argument("path1", help="First directory path (files from here can be deleted)")
    parser.add_argument("path2", help="Second directory path")
    parser.add_argument("--checkpoint", "-c", default="checkpoint.json",
                       help="Checkpoint file for interruption recovery (default: checkpoint.json)")
    
    args = parser.parse_args()
    
    # Validate paths
    if not os.path.isdir(args.path1):
        print(f"Error: {args.path1} is not a valid directory")
        sys.exit(1)
    
    if not os.path.isdir(args.path2):
        print(f"Error: {args.path2} is not a valid directory")
        sys.exit(1)
    
    # Run duplicate finder
    finder = DuplicateFinder(args.path1, args.path2, args.checkpoint)
    
    try:
        finder.run()
    except KeyboardInterrupt:
        print("\n\nInterrupted by user. Saving checkpoint...")
        finder._save_checkpoint()
        print(f"Checkpoint saved to {args.checkpoint}")
        print("Run the script again to continue from where you left off.")
        sys.exit(0)
    except Exception as e:
        print(f"\nError: {e}")
        print("Saving checkpoint before exit...")
        finder._save_checkpoint()
        sys.exit(1)

if __name__ == "__main__":
    main()
