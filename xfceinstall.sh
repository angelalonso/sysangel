#!/usr/bin/env bash

# TODO: Pipefail exits also if I choose not to install something.
#       Obviously response handling is wrong
#set -eo pipefail
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


vimcompile(){
  COMPILER=$(vim --version 2>/dev/null| grep "Compiled by" | awk '{print $3}')
  REAL=${USER}@${HOSTNAME}

  if [[ "${COMPILER}" == "${REAL}" ]]; then
    echo -e "${RED}Another compiled vim exists!${NC}"
    LOOP=true
    while [[ $LOOP == true ]] ; do
      read -r -n 1 -p "${1:-Do you want to RECOMPILE?} [y/n]: " REPLY
      case $REPLY in
        [yY])
          echo
          echo -e "${LGR}compiling vim${NC}"
          ${GITDIR}/scripts/vim_compile.sh install
          LOOP=false
          ;;
        [nN])
          echo
          LOOP=false
          ;;
        *) printf " \033[31m %s \n\033[0m" "invalid input"
      esac
    done
  else
    echo -e "${LGR}compiling vim${NC}"
    echo -e "${LBL}Press <Intro> when you are ready...${NC}"
    read confirm
    ${GITDIR}/scripts/vim_compile.sh install
  fi
}


# Dependencies
otherpackages(){
  # Needed: firefox for debian (compile from source?)
  # Franz
echo "here"
}


ohmyzsh(){
  if [[ -d ${HOME}/.oh-my-zsh ]]; then
    echo -e "${RED}Oh my Zsh is already installed!${NC}"
    LOOP=true
    while [[ $LOOP == true ]] ; do
      read -r -n 1 -p "${1:-Do you want to REINSTALL?} [y/n]: " REPLY
      case $REPLY in
        [yY])
          echo
          /usr/bin/xterm -e "echo 'IMPORTANT: \n when installation finishes, enter exit ON THE MAIN TERMINAL to continue'; read answer" &
          echo -e "${LGR}Reinstalling ohmyszh${NC}"
          ${GITDIR}/scripts/ohmyzsh.sh install
          LOOP=false
          ;;
        [nN])
          echo
          LOOP=false
          ;;
        *) printf " \033[31m %s \n\033[0m" "invalid input"
      esac
    done
  else
    echo -e "${LGR}installing ohmyszh${NC}"
    echo -e "${LBL}Press <Intro> when you are ready...${NC}"
    read confirm
    /usr/bin/xterm -e "echo 'IMPORTANT: \n when installation finishes, enter exit ON THE MAIN TERMINAL to continue'; read answer" &
    ${GITDIR}/scripts/ohmyzsh.sh install
  fi
}

# Keys

# Configs
configs(){

  ${GITDIR}/scripts/xfce_configs.sh install

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
for i in $(ls ${HOME}/.ssh | grep -v "pub\|cer\|config\|known" ); do chmod 0600 ${HOME}/.ssh/$i; ssh-add ${HOME}/.ssh/$i ; done


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
    vimcompile
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
      vimcompile)
        vimcompile
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
        echo "Usage: $0 [remove|packages|secrets|vimcompile|otherpackages|ohmyszh|configs|private_configs|to_do|cleanup]"
        ;;
    esac

  fi
fi
