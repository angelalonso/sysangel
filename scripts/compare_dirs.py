#!/usr/bin/env python3

"""
compare_dirs - Compare two directories for missing files and content differences
If a third parameter is provided, it compares only that specific file path.
"""

import os
import sys
import filecmp
from pathlib import Path
from collections import defaultdict
import hashlib
import subprocess

# ANSI color codes
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    END = '\033[0m'
    BOLD = '\033[1m'

def get_all_files(directory):
    """Get all files in directory recursively with relative paths"""
    files = set()
    for root, dirs, files_in_dir in os.walk(directory):
        rel_path = os.path.relpath(root, directory)
        for file in files_in_dir:
            if rel_path == '.':
                files.add(file)
            else:
                files.add(os.path.join(rel_path, file))
    return files

def get_file_hash(filepath, blocksize=65536):
    """Calculate SHA256 hash of a file"""
    hasher = hashlib.sha256()
    try:
        with open(filepath, 'rb') as f:
            for block in iter(lambda: f.read(blocksize), b''):
                hasher.update(block)
        return hasher.hexdigest()
    except (IOError, OSError) as e:
        return f"Error: {e}"

def compare_single_file(dir1, dir2, rel_path):
    """Compare a single file between two directories"""
    file1 = os.path.join(dir1, rel_path)
    file2 = os.path.join(dir2, rel_path)
    
    print(f"{Colors.BLUE}Comparing single file:{Colors.END}")
    print(f"  {Colors.GREEN}File: {rel_path}{Colors.END}")
    print(f"  {Colors.GREEN}Dir1: {dir1}{Colors.END}")
    print(f"  {Colors.GREEN}Dir2: {dir2}{Colors.END}")
    print()
    
    # Check if file exists in both directories
    exists1 = os.path.isfile(file1)
    exists2 = os.path.isfile(file2)
    
    if not exists1 and not exists2:
        print(f"{Colors.RED}Error: File doesn't exist in either directory{Colors.END}")
        return
    elif not exists1:
        print(f"{Colors.RED}File exists only in dir2:{Colors.END}")
        print(f"  {file2}")
        return
    elif not exists2:
        print(f"{Colors.RED}File exists only in dir1:{Colors.END}")
        print(f"  {file1}")
        return
    
    # Get file info
    size1 = os.path.getsize(file1)
    size2 = os.path.getsize(file2)
    
    print(f"{Colors.BLUE}File sizes:{Colors.END}")
    print(f"  Dir1: {size1} bytes")
    print(f"  Dir2: {size2} bytes")
    print()
    
    # Compare files
    if filecmp.cmp(file1, file2, shallow=False):
        print(f"{Colors.GREEN}Files are identical{Colors.END}")
    else:
        print(f"{Colors.RED}Files are different{Colors.END}")
        
        # Show hashes
        hash1 = get_file_hash(file1)
        hash2 = get_file_hash(file2)
        print(f"\n{Colors.BLUE}SHA256 hashes:{Colors.END}")
        print(f"  Dir1: {hash1}")
        print(f"  Dir2: {hash2}")
        
        # Try to show diff for text files
        try:
            # Check if it's likely a text file
            with open(file1, 'r') as f:
                f.read(1024)
            
            print(f"\n{Colors.YELLOW}Full diff output:{Colors.END}")
            print("=" * 80)
            
            # Run diff command
            try:
                result = subprocess.run(['diff', '-u', '--color=always', file1, file2], 
                                      capture_output=True, text=True)
                if result.stdout:
                    print(result.stdout)
                else:
                    # Try regular diff if unified diff is empty
                    result = subprocess.run(['diff', '--color=always', file1, file2], 
                                          capture_output=True, text=True)
                    print(result.stdout)
            except subprocess.TimeoutExpired:
                print("Diff timed out (file too large?)")
            except Exception as e:
                print(f"Error running diff: {e}")
            
            print("=" * 80)
            
        except (UnicodeDecodeError, Exception):
            print(f"\n{Colors.YELLOW}File appears to be binary - showing hexdump of first 256 bytes:{Colors.END}")
            try:
                with open(file1, 'rb') as f1, open(file2, 'rb') as f2:
                    data1 = f1.read(256)
                    data2 = f2.read(256)
                    
                    print(f"\n{Colors.BLUE}First 256 bytes of {rel_path} (dir1):{Colors.END}")
                    print(' '.join(f'{b:02x}' for b in data1))
                    print(f"\n{Colors.BLUE}First 256 bytes of {rel_path} (dir2):{Colors.END}")
                    print(' '.join(f'{b:02x}' for b in data2))
            except Exception as e:
                print(f"Error reading binary file: {e}")

def compare_directories(dir1, dir2, single_file=None):
    """Main comparison function"""
    
    # Normalize paths
    dir1 = os.path.abspath(dir1)
    dir2 = os.path.abspath(dir2)
    
    # If single file comparison requested
    if single_file:
        compare_single_file(dir1, dir2, single_file)
        return
    
    # Full directory comparison
    print(f"{Colors.BLUE}Comparing directories:{Colors.END}")
    print(f"  {Colors.GREEN}Dir1: {dir1}{Colors.END}")
    print(f"  {Colors.GREEN}Dir2: {dir2}{Colors.END}")
    print()
    
    # Get all files
    print(f"{Colors.YELLOW}=== PHASE 1: Missing Files Comparison ==={Colors.END}")
    print()
    
    files1 = get_all_files(dir1)
    files2 = get_all_files(dir2)
    
    # Files only in dir1
    print(f"{Colors.BLUE}Files only in {dir1}:{Colors.END}")
    only_in_1 = files1 - files2
    if only_in_1:
        for f in sorted(only_in_1):
            print(f"  {Colors.RED}{f}{Colors.END}")
    else:
        print("  (none)")
    print()
    
    # Files only in dir2
    print(f"{Colors.BLUE}Files only in {dir2}:{Colors.END}")
    only_in_2 = files2 - files1
    if only_in_2:
        for f in sorted(only_in_2):
            print(f"  {Colors.RED}{f}{Colors.END}")
    else:
        print("  (none)")
    print()
    
    # Phase 2
    print(f"{Colors.YELLOW}=== PHASE 2: Content Comparison ==={Colors.END}")
    input("Press Enter to continue with content comparison...")
    print()
    
    print(f"{Colors.BLUE}Files with different content:{Colors.END}")
    print()
    
    common_files = files1 & files2
    different_files = []
    
    for rel_path in sorted(common_files):
        file1 = os.path.join(dir1, rel_path)
        file2 = os.path.join(dir2, rel_path)
        
        # Skip if not regular files
        if not os.path.isfile(file1) or not os.path.isfile(file2):
            continue
            
        # Compare files
        if not filecmp.cmp(file1, file2, shallow=False):
            different_files.append(rel_path)
            
            # Get file info
            size1 = os.path.getsize(file1)
            size2 = os.path.getsize(file2)
            
            print(f"{Colors.RED}Different:{Colors.END} {rel_path}")
            print(f"  {Colors.YELLOW}Sizes:{Colors.END} {size1} vs {size2} bytes")
            
            # Show hashes for verification
            hash1 = get_file_hash(file1)[:16]  # First 16 chars of hash
            hash2 = get_file_hash(file2)[:16]
            if hash1 != hash2:
                print(f"  {Colors.YELLOW}Hashes:{Colors.END} {hash1}... vs {hash2}...")
            
            # Try to show first difference if text file
            try:
                with open(file1, 'r') as f1, open(file2, 'r') as f2:
                    # Check if it's likely a text file
                    f1.readline()
                    f2.readline()
                    # If we get here, it's probably text
                    print(f"  {Colors.YELLOW}First few lines of diff:{Colors.END}")
                    result = subprocess.run(['diff', '-u', file1, file2], 
                                          capture_output=True, text=True, timeout=1)
                    diff_lines = result.stdout.split('\n')[:5]
                    for line in diff_lines:
                        if line:
                            print(f"    {line}")
            except (UnicodeDecodeError, subprocess.TimeoutExpired, Exception):
                # Binary file or diff error - skip
                pass
            
            print()
    
    if not different_files:
        print("  All common files are identical")
    
    print()
    print(f"{Colors.GREEN}Comparison complete!{Colors.END}")
    print(f"Total different files: {len(different_files)}")

def main():
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print(f"Usage: {sys.argv[0]} <dir1> <dir2> [relative_file_path]")
        print(f"Examples:")
        print(f"  {sys.argv[0]} /path/to/dir1 /path/to/dir2")
        print(f"  {sys.argv[0]} /path/to/dir1 /path/to/dir2 path/to/file.txt")
        sys.exit(1)
    
    dir1, dir2 = sys.argv[1], sys.argv[2]
    single_file = sys.argv[3] if len(sys.argv) == 4 else None
    
    if not os.path.isdir(dir1):
        print(f"{Colors.RED}Error: Directory '{dir1}' does not exist{Colors.END}")
        sys.exit(1)
    
    if not os.path.isdir(dir2):
        print(f"{Colors.RED}Error: Directory '{dir2}' does not exist{Colors.END}")
        sys.exit(1)
    
    try:
        compare_directories(dir1, dir2, single_file)
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}Comparison interrupted by user{Colors.END}")
        sys.exit(1)

if __name__ == "__main__":
    main()
