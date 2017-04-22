#!/usr/bin/env bash

set -eo pipefail
# Vars

USR=$(whoami)
HOME="/home/${USR}"
INSTALLDIR="${HOME}/.sysangel"
TMPDIR="${INSTALLDIR}/tmp"
KEYSDIR="${INSTALLDIR}/keys"
GITDIR="${HOME}/sysangel"

PKGS="curl encfs exfat-fuse exfat-utils expect fabric git \
gtk2-engines-murrine gtk3-engines-xfce \
jq keepass2 openssh-client passwd pdftk pwgen \
python python-pip seahorse sudo tcptraceroute terminator \
unzip xbindkeys xvkbd zim zip zsh \
autokey-gtk"

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
  echo -e "${LGR}- Your Dropbox username and password"
  echo -e "${LBL}Press <Intro> when you are ready...${NC}"
  read confirm

  # BEFORE STARTING, create directories needed for the future
  mkdir -p ${INSTALLDIR}
  mkdir -p ${KEYSDIR}
  mkdir -p ${HOME}/Private
  mkdir -p ${HOME}/Private_offline
  # Create directories only for the installation
  mkdir -p ${TMPDIR}
  # , then continue to the next steps...
}


packages(){
  echo -e "${LGR}installing packages${NC}"
  sudo apt-get update && sudo apt-get install ${PKGS}
}


secrets(){
  echo -e "${LGR}installing keys and passwords${NC}"
  echo -e "${LBL}Press <Intro> when you are ready...${NC}"
  read confirm
  ${GITDIR}/scripts/xfce_secrets.sh install
}


vim(){
  echo -e "${LGR}installing vim${NC}"
  ${GITDIR}/scripts/vim_compile.sh install
}


# Dependencies
otherpackages(){
  # Needed: firefox for debian (compile from source?)
echo "here"
}


ohmyzsh(){
  /usr/bin/xterm -e "echo 'IMPORTANT: \n when installation finishes, enter exit ON THE MAIN TERMINAL to continue'; read answer" &
  echo -e "${LGR}installing ohmyszh${NC}"
  ${GITDIR}/scripts/ohmyzsh.sh install
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

if [[ ! -f ${HOME}/.config/terminator/config.orig ]]; then
  mv ${HOME}/.config/terminator/config ${HOME}/.config/terminator/config.orig 2>/dev/null
else
  rm ${HOME}/.config/terminator/config 2>/dev/null
fi
ln -s ${HOME}/Dropbox/data/config_open/terminator_config ${HOME}/.config/terminator/config

# Install ZSH config
echo -e "${LGR}installing ZSH config ${NC}"

if [[ ! -f ${HOME}/.zshrc.orig ]]; then
  mv ${HOME}/.zshrc ${HOME}/.zshrc.orig 2>/dev/null
else
  rm -rf ${HOME}/.zshrc 2>/dev/null
fi
ln -s ${HOME}/Dropbox/data/config_open/zshrc_home ${HOME}/.zshrc

# Autostart applications .desktop files
if [[ ! -f ${HOME}/.config/autostart ]]; then
  mv ${HOME}/.config/autostart ${HOME}/.config/autostart.orig 2>/dev/null
else
  rm -rf ${HOME}/.config/autostart 2>/dev/null
fi
ln -s ${GITDIR}/files/xfce_autostart ${HOME}/.config/autostart

# Xmodmap with Apple-like keys
if [[ ! -f ${HOME}/.Xmodmap ]]; then
  mv ${HOME}/.Xmodmap ${HOME}/.Xmodmap.orig 2>/dev/null
else
  rm -rf ${HOME}/.Xmodmap 2>/dev/null
fi
ln -s ${GITDIR}/files/Xmodmap_mac_ctrl ${HOME}/.Xmodmap

# Xbindkeys and xvkbd trik to use ctrl on terminal
if [[ ! -f ${HOME}/.xbindkeysrc ]]; then
  mv ${HOME}/.xbindkeysrc ${HOME}/.xbindkeysrc.orig 2>/dev/null
else
  rm -rf ${HOME}/.xbindkeysrc 2>/dev/null
fi
ln -s ${GITDIR}/files/xbindkeysrc ${HOME}/.xbindkeysrc


# xfce xml config files
if [[ ! -f ${HOME}/.config/xfce4/xfconf/xfce-perchannel-xml ]]; then
  mv ${HOME}/.config/xfce4/xfconf/xfce-perchannel-xml ${HOME}/.config/xfce4/xfconf/xfce-perchannel-xml 2>/dev/null
else
  rm -rf ${HOME}/.config/xfce4/xfconf/xfce-perchannel-xml 2>/dev/null
fi
ln -s ${GITDIR}/files/xfce/xfce-perchannel-xml ${HOME}/.config/xfce4/xfconf/xfce-perchannel-xml

}

# Private Configs
private_configs(){

# We mount to Private_offline by default.
#   , but it will be re-linked to the Private one, when it gets mounted

# Install SSH keys
echo -e "${LGR}installing ssh keys${NC}"

if [[ ! -f ${HOME}/.ssh.orig ]]; then
  mv ${HOME}/.ssh ${HOME}/.ssh.orig 2>/dev/null
else
  rm -rf ${HOME}/.ssh 2>/dev/null
fi
ln -s ${HOME}/Private_offline/config_secret/.ssh ${HOME}/.ssh

# Install AWS keys
echo -e "${LGR}installing aws keys${NC}"

if [[ ! -f ${HOME}/.aws.orig ]]; then
  mv ${HOME}/.aws ${HOME}/.aws.orig 2>/dev/null
else
  rm -rf ${HOME}/.aws 2>/dev/null
fi
ln -s ${HOME}/Private_offline/config_secret/.aws ${HOME}/.aws

# check the mount of Private again, solve links situation
${GITDIR}/scripts/privatemount.sh
}

to_do(){
echo -e "${RED} Further Manual Steps needed:"
echo
echo -e "${LBL} Apply the themes in:"
echo -e "${LGR} Settings Manager --> Appearance --> Style tab: choose 'Greybird master'
Settings Manager --> Appearance --> Icons tab: choose 'elementary xfce dark'
Settings Manager --> Window Manager --> Style tab: choose 'Greybird master'${NC}"

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
  echo -e "${NC}, then add something like the following line: ${ORN}"
  echo -e "${USR} ALL=(ALL:ALL) ALL${NC}"
  exit 2
else

  if [ "$#" -ne 1 ]; then
    preparation
    packages
    secrets
    vim
    otherpackages
    ohmyzsh
    configs
    private_configs
    to_do
    cleanup
  else
    case $1 in
      remove)
        ;;
      packages)
        packages
        ;;
      secrets)
        secrets
        ;;
      vim)
        vim
        ;;
      otherpackages)
        otherpackages
        ;;
      ohmyzsh)
        ohmyzsh
        ;;
      configs)
        configs
        ;;
      private_configs)
        private_configs
        ;;
      to_do)
        to_do
        ;;
      cleanup)
        cleanup
        ;;
      *)
        echo "Usage: $0 [remove|packages|secrets|vim|otherpackages|ohmyszh|configs|private_configs|to_do|cleanup]"
        ;;
    esac

  fi
fi
