import os
import subprocess
import shutil
import logging
from pathlib import Path
from config.config_manager import config_manager

class BackupEngine:
    def __init__(self):
        self.logger = logging.getLogger(__name__)
        self.rsync_engine = RsyncBackupEngine()
        self.use_rsync = self.rsync_engine.verify_rsync_available()
        
        if self.use_rsync:
            self.logger.info(f"Using rsync: {self.rsync_engine.get_rsync_version()}")
        else:
            self.logger.warning("Rsync not available, using Python fallback")
    
    def perform_backup(self, backup_type, compression, encryption, selected_tiers=None):
        """Perform backup using the best available method"""
        if self.use_rsync:
            return self.rsync_engine.perform_backup(backup_type, compression, 
                                                  encryption, selected_tiers)
        else:
            return self._perform_backup_fallback(backup_type, compression, 
                                               encryption, selected_tiers)
    
    def _perform_backup_fallback(self, backup_type, compression, encryption, selected_tiers):
        """Fallback backup using pure Python (basic implementation)"""
        try:
            configured_media = config_manager.get('backup.media', [])
            tiers_config = config_manager.get('backup.tiers', {})
            
            if not configured_media:
                raise Exception("No backup media configured")
            
            tiers_to_backup = selected_tiers or list(tiers_config.keys())
            
            for media in configured_media:
                if isinstance(media, dict):
                    media_path = media['path']
                    media_tiers = media.get('tiers', [])
                    
                    common_tiers = set(media_tiers) & set(tiers_to_backup)
                    if common_tiers:
                        self._backup_fallback_to_media(media_path, common_tiers, 
                                                     tiers_config)
            
            return True
            
        except Exception as e:
            self.logger.error(f"Fallback backup failed: {e}")
            return False
    
    def _backup_fallback_to_media(self, media_path, tiers, tiers_config):
        """Fallback backup implementation"""
        backup_dir = os.path.join(media_path, "BasicBackup")
        os.makedirs(backup_dir, exist_ok=True)
        
        for tier in tiers:
            tier_config = tiers_config.get(tier, {})
            self._backup_tier_fallback(tier_config, backup_dir, tier)
    
    def _backup_tier_fallback(self, tier_config, backup_dir, tier_name):
        """Fallback tier backup using shutil"""
        include_paths = tier_config.get('include', [])
        exclude_paths = tier_config.get('exclude', [])
        
        tier_backup_dir = os.path.join(backup_dir, tier_name)
        os.makedirs(tier_backup_dir, exist_ok=True)
        
        for source_path in include_paths:
            if os.path.exists(source_path) and not self._is_excluded(source_path, exclude_paths):
                try:
                    dest_path = os.path.join(tier_backup_dir, os.path.basename(source_path))
                    
                    if os.path.isfile(source_path):
                        shutil.copy2(source_path, dest_path)
                        self.logger.info(f"Copied file: {source_path} -> {dest_path}")
                    elif os.path.isdir(source_path):
                        # Simple directory copy (no exclusion handling in this basic version)
                        if os.path.exists(dest_path):
                            shutil.rmtree(dest_path)
                        shutil.copytree(source_path, dest_path)
                        self.logger.info(f"Copied directory: {source_path} -> {dest_path}")
                        
                except Exception as e:
                    self.logger.error(f"Failed to copy {source_path}: {e}")
    
    def _is_excluded(self, path, exclude_paths):
        """Check if path should be excluded"""
        for exclude in exclude_paths:
            if path.startswith(exclude):
                return True
        return False

class RsyncBackupEngine:
    # ... (the RsyncBackupEngine class from above goes here)
    def __init__(self):
        self.logger = logging.getLogger(__name__)
    
    def perform_backup(self, backup_type, compression, encryption, selected_tiers=None):
        """Perform backup using rsync"""
        try:
            # Get configured media and tiers
            configured_media = config_manager.get('backup.media', [])
            tiers_config = config_manager.get('backup.tiers', {})
            
            if not configured_media:
                raise Exception("No backup media configured")
            
            # Get tier data to backup
            tiers_to_backup = selected_tiers or list(tiers_config.keys())
            
            backup_results = []
            for media in configured_media:
                if isinstance(media, dict):
                    media_path = media['path']
                    media_tiers = media.get('tiers', [])
                    
                    # Only backup to media that has the selected tiers
                    common_tiers = set(media_tiers) & set(tiers_to_backup)
                    if common_tiers:
                        result = self._backup_to_media(media_path, common_tiers, 
                                                     backup_type, compression, 
                                                     encryption, tiers_config)
                        backup_results.append(result)
            
            return all(backup_results)
            
        except Exception as e:
            self.logger.error(f"Backup failed: {e}")
            return False
    
    def _backup_to_media(self, media_path, tiers, backup_type, compression, encryption, tiers_config):
        """Backup specific tiers to a media location using rsync"""
        try:
            backup_dir = os.path.join(media_path, "BasicBackup")
            os.makedirs(backup_dir, exist_ok=True)
            
            all_success = True
            for tier in tiers:
                tier_config = tiers_config.get(tier, {})
                success = self._backup_tier_rsync(tier_config, backup_dir, tier, 
                                                backup_type, compression)
                all_success = all_success and success
            
            return all_success
            
        except Exception as e:
            self.logger.error(f"Failed to backup to {media_path}: {e}")
            return False
    
    def _backup_tier_rsync(self, tier_config, backup_dir, tier_name, backup_type, compression):
        """Backup a single tier using rsync"""
        try:
            include_paths = tier_config.get('include', [])
            exclude_paths = tier_config.get('exclude', [])
            
            if not include_paths:
                self.logger.warning(f"No include paths configured for tier {tier_name}")
                return True
            
            tier_backup_dir = os.path.join(backup_dir, tier_name)
            os.makedirs(tier_backup_dir, exist_ok=True)
            
            all_success = True
            for source_path in include_paths:
                if os.path.exists(source_path):
                    success = self._rsync_backup_item(source_path, tier_backup_dir, 
                                                    exclude_paths, compression)
                    all_success = all_success and success
                else:
                    self.logger.warning(f"Source path does not exist: {source_path}")
            
            return all_success
            
        except Exception as e:
            self.logger.error(f"Failed to backup tier {tier_name}: {e}")
            return False
    
    def _rsync_backup_item(self, source_path, backup_dir, exclude_paths, compression):
        """Backup a single item using rsync"""
        try:
            # Build rsync command
            cmd = ['rsync', '-av', '--progress']
            
            # Add compression if enabled
            if compression:
                cmd.append('-z')
            
            # Add exclude patterns
            for exclude in exclude_paths:
                cmd.extend(['--exclude', exclude])
            
            # Handle trailing slashes for proper rsync behavior
            if source_path.endswith('/'):
                source_path = source_path[:-1]
            
            # Add source and destination
            cmd.append(source_path + '/')
            cmd.append(backup_dir + '/')
            
            self.logger.info(f"Running rsync: {' '.join(cmd)}")
            
            # Execute rsync
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=3600)
            
            if result.returncode == 0:
                self.logger.info(f"Successfully backed up: {source_path}")
                self.logger.debug(f"Rsync output: {result.stdout}")
                return True
            else:
                self.logger.error(f"Rsync failed for {source_path}: {result.stderr}")
                return False
                
        except subprocess.TimeoutExpired:
            self.logger.error(f"Rsync timeout for {source_path}")
            return False
        except Exception as e:
            self.logger.error(f"Unexpected error backing up {source_path}: {e}")
            return False
    
    def verify_rsync_available(self):
        """Check if rsync is available on the system"""
        try:
            result = subprocess.run(['rsync', '--version'], 
                                  capture_output=True, text=True)
            return result.returncode == 0
        except FileNotFoundError:
            return False
    
    def get_rsync_version(self):
        """Get rsync version if available"""
        try:
            result = subprocess.run(['rsync', '--version'], 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.split('\n')[0]
            return "Unknown"
        except FileNotFoundError:
            return "Not available"
