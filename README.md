# Sysangel

Scripts to get my computers up and running with as little manual effort as possible

# Installation Script

Installs minimum programs I require after a fresh OS install.
Feel free to modify it to your liking.

## Rpeparations and running it
Tested for Ubuntu:
```sudo apt update
sudo apt upgrade 
sudo apt install git
cd ~ && git clone https://github.com/angelalonso/sysangel && cd sysangel
bash install.sh
```

## Status
A first proper draft is being tested right now

# Backup Scripts

Copies over a list of folders from my computer to a mounted drive.

## Requirements
- This thing runs on Linux
- You'll need rsync installed
- Copy over backup.list to the root of your mounted drive (the one you want to save your backups to)
  - Modify the list to your likings
- Modify backup.sh and backup_wrapped.sh, for instance, if the drive you backup to is not mounted under /media

## How to run it
```
./backup.sh <MOUNT_FOLDER>
```
, where FOLDER_NAME is the name of the folder below /media/$USER where the drive to backup to is mounted (E.g.: /media/user/MYHDD would be used with ./backup.sh MYHDD)


