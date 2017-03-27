#!/usr/bin/env bash

# Vars

USR=$(whoami)
HOME="/home/${USR}"
INSTALLDIR="${HOME}/.sysangel"
TMPDIR="${INSTALLDIR}/tmp"
KEYSDIR="${INSTALLDIR}/keys"
GITDIR="${HOME}/sysangel"

PKGS="curl encfs exfat-fuse exfat-utils expect fabric iceweasel git \
gtk2-engines-murrine gtk3-engines-xfce \
jq keepass2 openssh-client passwd pdftk pwgen \
python python-pip sudo tcptraceroute terminator \
zim zsh"

# Bash colors
BLU='\033[0;34m'
LGR='\033[1;32m'
LBL='\033[1;34m'
ORN='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check it's the right system

# if [ $(uname) == "Linux" ]; then
#   SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)
# else
#   exit 2
# fi

# Get sudo
# Add user to sudoers
# TODO: this gives me an error (-i not recognized?)
# if [ $(su -i env USRin="${USR}" sh -c 'grep ${USRin} /etc/sudoers | wc -l') -lt 1 ]; then
#   su -i env USRin="${USR}" sh -c 'echo "${USRin} ALL=(ALL:ALL) ALL" >> /etc/sudoers'
# fi

preparation(){
echo -e "${LBL} Before we start, bear in mind that you will need the following:"
echo -e "${LGR}- Your Encrypted Dropbox folder's password"
echo -e "${LBL}Press <Intro> when you are ready...${NC}"
read confirm
}

# Dependencies
otherpackages(){
echo "here"
}

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
echo -e "${LGR}installing theme${NC}"
echo -e "${LBL}Press <Intro> when you are ready...${NC}"
read confirm
mkdir -p ~/.themes
cd $_
wget https://github.com/shimmerproject/Greybird/archive/master.zip
unzip master.zip
rm master.zip


# Install Icons
echo -e "${LGR}installing icons${NC}"
mkdir -p ~/.icons
cd $_
wget https://github.com/shimmerproject/elementary-xfce/archive/master.zip
unzip master.zip
mv elementary*/* .
rm master.zip
# update icon cache (optional)
gtk-update-icon-cache-3.0 -f -t ~/.icons
echo -e "${LBL} Apply the themes in:"
echo -e "${LGR} Settings Manager --> Appearance --> Style tab: choose 'Greybird master'
Settings Manager --> Appearance --> Icons tab: choose 'elementary xfce dark'
Settings Manager --> Window Manager --> Style tab: choose 'Greybird master'${NC}"
#For the themes to work even for the root user (such as Synaptic), you have to set up a symlink:
sudo ln -s /home/aaf/.themes  /root


# Install Fonts
echo -e "${LGR}installing fonts${NC}"
# Thanks to https://github.com/belluzj/fantasque-sans
# I already compiled my own, I'd rather just copy it
sudo mkdir -p /usr/local/share/fonts/f
sudo cp ${GITDIR}/files/FantasqueSansMono_Regular.ttf /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf
sudo chmod 644 /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf
sudo chown root:staff /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf


# Install Terminator config
echo -e "${LGR}installing terminator config${NC}"
mkdir -p ${HOME}/.config/terminator
# Don't overwrite previous backups
if [[ -f ${HOME}/.config/terminator/config && ! -f ${HOME}/.config/terminator/config.orig ]]; then
  cp ${HOME}/.config/terminator/config ${HOME}/.config/terminator/config.orig
fi
ln -s ${GITDIR}/files/terminator_config ${HOME}/.config/terminator/config

#TODO: is this needed?
#cp ${FILESDIR}/capstoesc.desktop ${HOME}.config/autostart/capstoesc.desktop


}

cleanup(){
# Remove directories that were created for the installation
rm -r ${TMPDIR}
}

AMISUDO=$(sudo -v 2>/dev/null; echo $?)
if [ $AMISUDO -ne 0 ]; then
  echo -e "${RED} Your user is NOT in the SUDOERS LIST!${NC}"
  echo " Please add it with:"
  echo -e "${ORN}su -l root"
  echo -e "visudo"
  echo -e "${NC}, then add the following line: ${ORN}"
  echo -e "${USR} ALL=(ALL:ALL) ALL${NC}"
  exit 2
else
  preparation
  echo -e "${LGR}installing packages${NC}"
  sudo apt-get update && sudo apt-get install ${PKGS}
#   echo -e "${LGR}installing keys and passwords${NC}"
#   ./scripts/secrets.sh install
#   echo -e "${LGR}installing vim${NC}"
#   ./scripts/vim_compile.sh install
#   echo -e "${LGR}installing ohmyszh${NC}"
#   ./scripts/ohmyzsh.sh install
  # TODO
  #otherpackages
#  configs
fi
