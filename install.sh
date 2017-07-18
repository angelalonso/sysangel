#!/usr/bin/env bash

# TODO: Pipefail exits also if I choose not to install something.
#       Obviously response handling is wrong
#set -eo pipefail
# Vars

ARCH=$(uname -m)
USR=$(whoami)
HOME="/home/${USR}"
INSTALLDIR="${HOME}/.sysangel"
TMPDIR="${INSTALLDIR}/tmp"
KEYSDIR="${INSTALLDIR}/keys"
GITDIR="${HOME}/sysangel"
FILESDIR="${GITDIR}/files"


GEN_PKGS="autokey-gtk python-boto3 curl \
encfs exfat-fuse exfat-utils expect \
fabric faenza-icon-theme git gtk2-engines-murrine gtk3-engines-xfce \
iotop jq keepassx mtr nmap openssh-client passwd pdftk pwgen \
python python-pip python-pip seahorse sudo tcptraceroute terminator \
unzip xbindkeys xvkbd zim zip zsh"

DEB_PKGS="chromium"
UBU_PKGS="compizconfig-settings-manager gnome-tweak-tool google-chrome-stable \
indicator-multiload ppa-purge"

# Bash colors
BLU='\033[0;34m'
LGR='\033[1;32m'
LBL='\033[1;34m'
ORN='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color


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
  case $DIST in
    Debian)
      if [[ ! -f /etc/apt/sources.list.orig ]]; then
        sudo mv /etc/apt/sources.list /etc/apt/sources.list.orig 2>/dev/null
      fi
      sudo cp ${FILESDIR}/debian/apt/sources.list /etc/apt/sources.list
      sudo cp ${FILESDIR}/debian/apt/virtualbox.list /etc/apt/sources.list.d/virtualbox.list

      PKGS="$GEN_PKGS $DEB_PKGS"
      ;;
    Ubuntu)
      if [[ ! -f /etc/apt/sources.list.orig ]]; then
        sudo mv /etc/apt/sources.list /etc/apt/sources.list.orig 2>/dev/null
      fi
      sudo cp ${FILESDIR}/ubuntu/apt/sources.list /etc/apt/sources.list
      #TODO: avoid doing this if they are already there
      sudo apt-key adv --keyserver pgp.mit.edu --recv-keys 5044912E
      wget -q https://dl.google.com/linux/linux_signing_key.pub -O- | sudo apt-key add -
      wget -q http://download.virtualbox.org/virtualbox/debian/oracle_vbox_2016.asc -O- | sudo apt-key add -
      sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 883E8688397576B6C509DF495A9A06AEF9CB8DB0

      PKGS="$GEN_PKGS $UBU_PKGS"
      ;;
  esac

  echo -e "${LGR}installing packages${NC}"
  sudo apt update && sudo apt install ${PKGS}
}


secrets(){
  echo -e "${LGR}installing keys and passwords${NC}"
  echo -e "${LBL}Press <Intro> when you are ready...${NC}"
  read confirm
  case $DIST in
    Debian)
      ${GITDIR}/scripts/debian_secrets.sh install
      ;;
    Ubuntu)
      ${GITDIR}/scripts/ubuntu_secrets.sh install
      ;;
  esac
}


vimcompile(){
  #TODO: Move these checks to the script itself
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
  # Let's count python's pip as packages as well
  PIP3_PACKS="flake8"
  sudo pip3 install ${PIP3_PACKS}


  KUBEEXE=$(which kubectl)
  if [[ ${KUBEEXE} == "" ]]; then
    echo -e "${LGR}installing Kubectl${NC}"
    echo -e "${LBL}Press <Intro> when you are ready...${NC}"
    read confirm
    ${GITDIR}/scripts/kubectl_config.sh install
  else
    echo -e "${RED}Kubectl is already installed!${NC}"
    LOOP=true
    while [[ $LOOP == true ]] ; do
      read -r -n 1 -p "${1:-Do you want to REINSTALL?} [y/n]: " REPLY
      case $REPLY in
        [yY])
          echo
          echo -e "${LGR}Reinstalling Kubectl${NC}"
          ${GITDIR}/scripts/kubectl_config.sh install
          LOOP=false
          ;;
        [nN])
          echo
          LOOP=false
          ;;
        *) printf " \033[31m %s \n\033[0m" "invalid input"
      esac
    done
  fi

  # Needed: firefox for debian (compile from source?)


  # Franz
  # Taken from https://gist.github.com/ruebenramirez/22234da93f08be65125cc45fc386c1cd
  FRANZEXE=$(which Franz)
  if [[ ${FRANZEXE} == "" ]]; then
    sudo rm -fr /opt/franz
    sudo rm -fr /usr/share/applications/franz.desktop
    # create installation dir
    sudo mkdir -p /opt/franz
    if test "${ARCH#*"64"}" != "$ARCH"; then
      wget -qO- https://github.com/meetfranz/franz-app/releases/download/4.0.4/Franz-linux-x64-4.0.4.tgz | sudo tar xvz -C /opt/franz/
    else
      wget -qO- https://github.com/meetfranz/franz-app/releases/download/4.0.4/Franz-linux-ia32-4.0.4.tgz | sudo tar xvz -C /opt/franz/
    fi
    sudo ln -s /opt/franz/Franz /usr/bin/Franz
    # add app icon
    sudo wget "https://cdn-images-1.medium.com/max/360/1*v86tTomtFZIdqzMNpvwIZw.png" -O /opt/franz/franz-icon.png
    # configure app for desktop use
    sudo bash -c "cat <<EOF > /usr/share/applications/franz.desktop
    [Desktop Entry]
    Name=Franz
    Comment=
    Exec=/opt/franz/Franz
    Icon=/opt/franz/franz-icon.png
    Terminal=false
    Type=Application
    Categories=Messaging,Internet
    EOF"
  else
    echo "Franz already installed"
  fi

  # Terraform
  TFEXE=$(which terraform)
  if [[ ${TFEXE} == "" ]]; then
    if test "${ARCH#*"64"}" != "$ARCH"; then
      wget https://releases.hashicorp.com/terraform/0.9.6/terraform_0.9.6_linux_amd64.zip
    elif test "${ARCH#*"86"}" != "$ARCH"; then
      wget https://releases.hashicorp.com/terraform/0.9.6/terraform_0.9.6_linux_386.zip
    elif test "${ARCH#*"arm"}" != "$ARCH"; then
      wget https://releases.hashicorp.com/terraform/0.9.6/terraform_0.9.6_linux_arm.zip
    fi
    unzip terraform_*
    sudo mv terraform /usr/local/bin/
    rm terraform_*
  else
    echo "Terraform already installed"
  fi

  # Vagrant
  VGEXE=$(which vagrant)
  if [[ ${VGEXE} == "" ]]; then
    if test "${ARCH#*"64"}" != "$ARCH"; then
      wget https://releases.hashicorp.com/vagrant/1.9.5/vagrant_1.9.5_x86_64.deb
    else
      wget https://releases.hashicorp.com/vagrant/1.9.5/vagrant_1.9.5_i686.deb
    fi
    sudo dpkg -i vagrant_*
    rm vagrant_*
  else
    echo "Vagrant already installed"
  fi

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

additional(){
  echo -e "${LBL} The following packages are not installed by default,"
  echo -e " Please confirm that you want to install each of them."

  #Docker
  echo -e "${RED}Docker is going to be installed!${NC}"
  LOOP=true
  while [[ $LOOP == true ]] ; do
    read -r -n 1 -p "${1:-DOCKER, do you want to INSTALL it?} [y/n]: " REPLY
    case $REPLY in
      [yY])
        echo
        # ONLY AVAILABLE FOR ARM AND AMD64, TODO: check this is the case or skip and ALERT
        echo -e "${LGR}installing DOCKER${NC}"
        ${GITDIR}/scripts/docker.sh install
        LOOP=false
        ;;
      [nN])
        echo
        LOOP=false
        ;;
      *) printf " \033[31m %s \n\033[0m" "invalid input"
    esac
  done
  # Virtualbox
  echo -e "${RED}Virtualbox is going to be installed!${NC}"
  LOOP=true
  while [[ $LOOP == true ]] ; do
    read -r -n 1 -p "${1:-VIRTUALBOX, do you REALLY want to INSTALL it?} [y/n]: " REPLY
    case $REPLY in
      [yY])
        echo
        echo -e "${LGR}Installing Virtualbox${NC}"
        curl -O https://www.virtualbox.org/download/oracle_vbox_2016.asc
        sudo apt-key add oracle_vbox_2016.asc
        sudo apt update
        sudo apt install virtualbox-5.1
        LOOP=false
        ;;
      [nN])
        echo
        LOOP=false
        ;;
      *) printf " \033[31m %s \n\033[0m" "invalid input"
    esac
  done
  #AWSCLI
  echo -e "${RED}AWSCLI is going to be installed!${NC}"
  LOOP=true
  while [[ $LOOP == true ]] ; do
    read -r -n 1 -p "${1:-AWSCLI, do you want to INSTALL it?} [y/n]: " REPLY
    case $REPLY in
      [yY])
        echo
        echo -e "${LGR}installing AWSCLI${NC}"
        pip install awscli --upgrade --user
        LOOP=false
        ;;
      [nN])
        echo
        LOOP=false
        ;;
      *) printf " \033[31m %s \n\033[0m" "invalid input"
    esac
  done



}


# Configs
configs(){

  ${GITDIR}/scripts/xfce_configs.sh install
  # Needed to improve terminator's tabs
  if [[ ! -f ${HOME}/.config/gtk-3.0/gtk.css.orig ]]; then
    mv ${HOME}/.config/gtk-3.0/gtk.css ${HOME}/.config/gtk-3.0/gtk.css.orig 2>/dev/null
  fi
  cp ${FILESDIR}/xfce/gtk-3.0_gtk.css ${HOME}/.config/gtk-3.0/gtk.css

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
for i in $(ls ${HOME}/.ssh/ | grep -v "pub\|cer\|config\|known" ); do chmod 0600 ${HOME}/.ssh/$i; ssh-add ${HOME}/.ssh/$i ; done


# Install AWS keys
echo -e "${LGR}installing aws keys${NC}"

if [[ ! -f ${HOME}/.aws.orig ]]; then
  mv ${HOME}/.aws ${HOME}/.aws.orig 2>/dev/null
else
  rm -rf ${HOME}/.aws 2>/dev/null
fi
ln -s ${HOME}/Private_offline/config_secret/.aws ${HOME}/.aws

# Install Kubectl keys
echo -e "${LGR}installing Kubectl keys${NC}"

if [[ ! -f ${HOME}/.kube.orig ]]; then
  mv ${HOME}/.kube ${HOME}/.kube.orig 2>/dev/null
else
  rm -rf ${HOME}/.kube 2>/dev/null
fi
ln -s ${HOME}/Private_offline/config_secret/.kube ${HOME}/.kube

# check the mount of Private again, solve links situation
${GITDIR}/scripts/privatemount.sh
}

to_do(){
  if [[ $DIST == "Ubuntu" ]]; then
    echo -e "${RED} Further Manual Steps for UBUNTU users:"
    echo
    echo -e "${LBL} - USE FAENZY ICONS:"
    echo -e "${LGL} Open gnome-tweak-tool > Appearance > Icons"
    echo -e "${LGR} Choose Faenza-Ambiance"
    echo
    echo -e "${LBL} - MAKE CAPS LOCK ANOTHER ESC:"
    echo -e "${LGL} Open gnome-tweak-tool > Typing > Caps Lock key behavior"
    echo -e "${LGR} Make Caps Lock an additional ESC"
    echo
    echo -e "${LBL} - AVOID COLLISION WITH <SUPER> KEY:"
    echo -e "${LGL} Open compizconfig-settings-manager or /usr/bin/python /usr/bin/ccsm"
    echo -e "${LGL} Ubuntu Unity Plugin > Launcher"
    echo -e "${LGR} Key to show the Dash... > <Alt><Super>"
    echo -e "${LGR} Key to start the Application switcher > Disabled"
    echo -e "${LGR} Key to start the App... in reverse > Disabled"
    echo
    echo -e "${LBL} - SIMULATE MAC COMMAND RUN:"
    echo -e "${LGL} Open compizconfig-settings-manager or /usr/bin/python /usr/bin/ccsm"
    echo -e "${LGL} Ubuntu Unity Plugin > General"
    echo -e "${LGR} Key to execute a command > <Shift><Control>XF86PowerOff"
    echo
    echo -e "${LBL} - SIMULATE MAC SWITCHER COMBO:"
    echo -e "${LGL} Open compizconfig-settings-manager or /usr/bin/python /usr/bin/ccsm"
    echo -e "${LGL} Ubuntu Unity Plugin > Switcher"
    echo -e "${LGR} Key to Start the Switcher > <Super>Tab"
    echo -e "${LGR} Key to Switch to the previous... > <Shift><Super>Tab"
    echo
    echo -e "${LBL} - SIMULATE MAC SCREEN LOCK COMBO:"
    echo -e "${LGL} Open compizconfig-settings-manager or /usr/bin/python /usr/bin/ccsm"
    echo -e "${LGL} Ubuntu Unity Plugin > General"
    echo -e "${LGR} Key to lock the screen > <Shift><Control>XF86PowerOff"
    echo
  fi

  echo -e "${RED} Further Manual Steps needed:"
  echo
  echo -e "${LBL} Install the following packages if the system is big enough:"
  echo -e "${LGR} sudo apt install \\"
  echo -e "${LGR} inkscape gimp libreoffice "
  echo
  echo -e "${LBL} Install the following packages if the system has BLUETOOTH:"
  echo -e "${LGR} sudo apt install \\"
  echo -e "${LGR} blueman firmware-atheros"
  echo
  echo -e "${LBL} If you are using an SSD Drive:"
  echo -e "${LGR} sudo vim /etc/fstab "
  echo -e "${LBL} , then add noatime to all \(root, home\) mountpoints EXCEPT the RAM one!"
  echo -e "${LBL} https://sites.google.com/site/easylinuxtipsproject/ssd#TOC-Avoid-exaggerated-measures"
  echo
  echo -e "${LBL} Then restart! ${NC}"

}

cleanup(){
# Remove directories that were created for the installation
rm -r ${TMPDIR} 2>/dev/null
}

# Find out the distro you are on
if [[ $(uname -a) == *"Debian"* ]]; then
  DIST="Debian"
elif [[ $(uname -a) == *"buntu"* ]]; then
  DIST="Ubuntu"
else
  DIST="Other"
fi

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
    additional
    configs
    private_configs
    to_do
    cleanup
  else
    case $1 in
      remove)
        echo "Not yet supported. Sorry"
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
      additional)
        additional
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
        echo "Usage: $0 [remove|packages|secrets|vimcompile|otherpackages|ohmyszh|configs|private_configs|to_do|additional|cleanup]"
        ;;
    esac

  fi
fi
