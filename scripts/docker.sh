#!/usr/bin/env bash

# Installs all parts required for the private mounpoint to work automatically

USR=$(whoami)
HOME="/home/${USR}"
INSTALLDIR="${HOME}/.sysangel"
GITDIR="${HOME}/sysangel"


# Bash colors
BLU='\033[0;34m'
LGR='\033[1;32m'
LBL='\033[1;34m'
ORN='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

install(){
  ARCH=$(uname -m)

  # Find out the distro you are on
  if [[ $(uname -a) == *"Debian"* ]]; then
    DIST="Debian"
    if test "${ARCH#*"64"}" != "$ARCH"; then
      echo "Installing docker for 64 bit Arch"
      sudo apt-get remove docker docker-engine
      sudo apt-get install apt-transport-https ca-certificates \
         curl gnupg2 software-properties-common
      curl -fsSL https://download.docker.com/linux/debian/gpg | sudo apt-key add -
      sudo apt-key fingerprint 0EBFCD88

      sudo add-apt-repository \
        "deb [arch=amd64] https://download.docker.com/linux/debian \
        $(lsb_release -cs) stable"
      sudo apt-get update
      sudo apt-get install docker-ce
    elif test "${ARCH#*"arm"}" != "$ARCH"; then
      echo "Installing docker for ARM Arch"
      sudo apt-get remove docker docker-engine
      sudo apt-get install apt-transport-https ca-certificates \
         curl gnupg2 software-properties-common
      curl -fsSL https://download.docker.com/linux/debian/gpg | sudo apt-key add -
      sudo apt-key fingerprint 0EBFCD88

      echo "deb [arch=armhf] https://download.docker.com/linux/debian \
        $(lsb_release -cs) stable" | \
        sudo tee /etc/apt/sources.list.d/docker.list
      sudo apt-get update
      sudo apt-get install docker-ce
    else
      echo "Docker has no installation candidate for your Architecture type, ${ARCH}"
    fi
  elif [[ $(uname -a) == *"buntu"* ]]; then
    DIST="Ubuntu"
    # TODO: Check also the arch here, maybe join with debian side
    sudo apt-get update
    sudo apt-get install apt-transport-https ca-certificates curl software-properties-common

    curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -

    sudo apt-key fingerprint 0EBFCD88

    sudo add-apt-repository \
      "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
      $(lsb_release -cs) \
      stable"
    sudo apt-get update

    sudo apt-get install docker-ce
  else
    DIST="Other"
  fi



}

remove(){
  echo "removing Docker"

}


case "$1" in
  install|i|Install|I)
    install
    ;;
  remove|Remove|r|R|uninstall|u|Uninstall|U)
    remove
    ;;
  *)
    echo "ERROR: Syntax is $0 [install|remove|]"
    ;;
esac
