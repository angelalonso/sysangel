#!/usr/bin/env bash

# Vars

USR=$(whoami)
INSTALLDIR="${HOME}/.sysangel"
TMPDIR="${INSTALLDIR}/tmp"
KEYSDIR="${INSTALLDIR}/keys"

PKGS="curl encfs exfat-fuse exfat-utils expect fabric git jq keepassx openssh-client passwd pdftk pwgen \
python python-pip sudo tcptraceroute terminator \
zim zsh"

# Check it's the right system

if [ $(uname) == "Linux" ]; then
  SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)
else; then
  exit 2
fi

# Get sudo
# Add user to sudoers
# TODO: this gives me an error (-i not recognized?)
if [ $(su -i env USRin="${USR}" sh -c 'grep ${USRin} /etc/sudoers | wc -l') -lt 1 ]; then
  su -i env USRin="${USR}" sh -c 'echo "${USRin} ALL=(ALL:ALL) ALL" >> /etc/sudoers'
fi

# Dependencies

# Folders
# Create directories needed for the future
mkdir -p ${INSTALLDIR}
mkdir -p ${KEYSDIR}
# Create directories only for the installation
mkdir -p ${TMPDIR}

# Keys

# Configs

# Cleanup
# Remove directories that were created for the installation
rm -r ${TMPDIR}

