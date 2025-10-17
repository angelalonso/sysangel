import os
import platform
import subprocess
import logging
from typing import List

def get_available_drives() -> List[str]:
    """Get list of available drives on the system for selection"""
    logger = logging.getLogger(__name__)
    logger.info("Detecting available drives for media selection...")
    drives = []
    system = platform.system()
    
    try:
        if system == "Windows":
            logger.info("Windows system detected")
            import string
            for drive_letter in string.ascii_uppercase:
                drive_path = f"{drive_letter}:\\"
                if os.path.exists(drive_path):
                    drives.append(drive_path)
        
        elif system == "Linux":
            logger.info("Linux system detected")
            # System directories to exclude
            system_dirs = ['/dev', '/proc', '/sys', '/run', '/snap', '/boot', '/boot/efi']
            
            # Use df command to find mounted filesystems (most reliable)
            try:
                result = subprocess.run(['df', '-h', '--output=target,size,avail'], 
                                      capture_output=True, text=True, timeout=5)
                if result.returncode == 0:
                    lines = result.stdout.split('\n')[1:]  # Skip header
                    for line in lines:
                        if line.strip():
                            parts = line.split()
                            if len(parts) >= 3:
                                mount_point = parts[0]
                                size = parts[1]
                                avail = parts[2]
                                
                                # Filter out system directories and root filesystem
                                if (mount_point != '/' and 
                                    not any(mount_point.startswith(sys_dir) for sys_dir in system_dirs) and
                                    not mount_point.startswith('/var/lib/') and
                                    not mount_point.startswith('/tmp') and
                                    os.path.ismount(mount_point)):
                                    
                                    # Only include common user-accessible locations
                                    if (mount_point.startswith('/media/') or 
                                        mount_point.startswith('/mnt/') or
                                        mount_point.startswith('/run/media/') or
                                        mount_point.startswith('/home/')):
                                        drives.append(mount_point)
                                        logger.info(f"Found user media via df: {mount_point}")
            except (subprocess.TimeoutExpired, FileNotFoundError, subprocess.SubprocessError) as e:
                logger.warning(f"df command failed: {e}")
            
            # Fallback: Check common media directories
            media_dirs = ['/media', '/mnt', '/run/media']
            for media_dir in media_dirs:
                if os.path.exists(media_dir):
                    try:
                        for item in os.listdir(media_dir):
                            full_path = os.path.join(media_dir, item)
                            if (os.path.ismount(full_path) and 
                                full_path not in drives and
                                not any(full_path.startswith(sys_dir) for sys_dir in system_dirs)):
                                drives.append(full_path)
                                logger.info(f"Found media in {media_dir}: {full_path}")
                    except (PermissionError, OSError) as e:
                        logger.warning(f"Could not access {media_dir}: {e}")
            
            # Also check user's home directory for potential backup locations
            try:
                home_dir = os.path.expanduser("~")
                # Check if home is on a separate mount point (common in some setups)
                if (os.path.ismount(home_dir) and 
                    home_dir not in drives and
                    home_dir != '/'):
                    drives.append(home_dir)
                    logger.info(f"Found home directory as separate mount: {home_dir}")
            except Exception as e:
                logger.warning(f"Could not check home directory: {e}")
            
            # Remove duplicates and sort
            drives = sorted(list(set(drives)))
            
        elif system == "Darwin":  # macOS
            logger.info("macOS system detected")
            volumes_dir = '/Volumes'
            if os.path.exists(volumes_dir):
                try:
                    for item in os.listdir(volumes_dir):
                        full_path = os.path.join(volumes_dir, item)
                        if os.path.ismount(full_path):
                            # Exclude system volumes on macOS
                            if not item.startswith('.') and item != 'Macintosh HD':
                                drives.append(full_path)
                except PermissionError:
                    logger.warning("Permission denied accessing /Volumes")
        
        logger.info(f"Total available drives found: {len(drives)}")
        return drives
        
    except Exception as e:
        logger.error(f"Error detecting drives: {e}")
        return []

def get_media_info(media_path: str) -> dict:
    """Get information about a media path (type, free space, total space)"""
    try:
        system = platform.system()
        info = {'path': media_path, 'type': 'Unknown', 'free_gb': 0, 'total_gb': 0, 'error': None}
        
        if system == "Windows":
            import ctypes
            drive_type = ctypes.windll.kernel32.GetDriveTypeW(media_path)
            type_names = {2: "Removable", 3: "Fixed", 4: "Remote", 5: "CD-ROM"}
            info['type'] = type_names.get(drive_type, "Unknown")
            
            # Get free space
            free_bytes = ctypes.c_ulonglong(0)
            total_bytes = ctypes.c_ulonglong(0)
            if ctypes.windll.kernel32.GetDiskFreeSpaceExW(
                ctypes.c_wchar_p(media_path), 
                None, 
                ctypes.pointer(total_bytes), 
                ctypes.pointer(free_bytes)
            ):
                info['free_gb'] = free_bytes.value / (1024**3)
                info['total_gb'] = total_bytes.value / (1024**3)
        
        else:
            # Linux/macOS
            stat = os.statvfs(media_path)
            info['free_gb'] = (stat.f_bavail * stat.f_frsize) / (1024**3)
            info['total_gb'] = (stat.f_blocks * stat.f_frsize) / (1024**3)
            
            # Determine media type based on path
            if '/media/' in media_path or '/run/media/' in media_path:
                info['type'] = "Removable Media"
            elif '/mnt/' in media_path:
                info['type'] = "Mounted Storage"
            elif media_path.startswith('/home/'):
                info['type'] = "Home Directory"
            else:
                info['type'] = "Storage"
        
        return info
        
    except Exception as e:
        return {'path': media_path, 'type': 'Unknown', 'free_gb': 0, 'total_gb': 0, 'error': str(e)}
