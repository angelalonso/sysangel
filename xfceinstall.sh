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
else
  exit 2
fi

# Get sudo
# Add user to sudoers
# TODO: this gives me an error (-i not recognized?)
if [ $(su -i env USRin="${USR}" sh -c 'grep ${USRin} /etc/sudoers | wc -l') -lt 1 ]; then
  su -i env USRin="${USR}" sh -c 'echo "${USRin} ALL=(ALL:ALL) ALL" >> /etc/sudoers'
fi

# Dependencies

folders(){
# Create directories needed for the future
mkdir -p ${INSTALLDIR}
mkdir -p ${KEYSDIR}
# Create directories only for the installation
mkdir -p ${TMPDIR}
}

# Keys

# Configs
configs(){

# Install theme
mkdir -p ~/.themes
cd $_
wget https://github.com/shimmerproject/Greybird/archive/master.zip
unzip master.zip
rm master.zip


# Install Icons
mkdir -p ~/.icons
cd $_

wget https://github.com/shimmerproject/elementary-xfce/archive/master.zip
unzip master.zip
mv elementary*/* .
rm master.zip

# update icon cache (optional)
gtk-update-icon-cache-3.0 -f -t ~/.icons

#For the Greybird theme to work, you need to:

apt-get install gtk2-engines-murrine gtk3-engines-unico

#Then apply the themes in:

#Settings Manager --> Appearance --> Style tab: choose "Greybird master"
#Settings Manager --> Appearance --> Icons tab: choose "elementary xfce dark"
#Settings Manager --> Window Manager --> Style tab: choose "Greybird master"

#For the themes to work even for the root user (such as Synaptic), you can set up a symlink:

# Open a root shell
su -
ln -s /home/aaf/.themes  /root

# Just checking
ls -A /root/.themes
#Greybird-master

}

cleanup(){
# Remove directories that were created for the installation
rm -r ${TMPDIR}
}
