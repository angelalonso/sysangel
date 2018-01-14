#!/usr/bin/env bash

#If a command fails, make the whole script exit
#Treat unset variables as an error, and immediately exit.
#Disable filename expansion (globbing) upon seeing *, ?, etc..
#Produce a failure return code if any command errors
set -euf -o pipefail

CURR_DIR=$(dirname $0)
ARCH=$(uname -m)

# Bash colors
BLU='\033[0;34m'
LGR='\033[1;32m'
LBL='\033[1;34m'
ORN='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color
 
pkgs-aptget() {

  PKGS="\
  curl \
  exfat-fuse \
  exfat-utils \
  expect \
  fabric \
  git \
  iotop \
  jq \
  keepassx \
  mtr \
  nmap \
  openssh-client \
  passwd \
  pwgen \
  python \
  python-pip \
  python-boto3 \
  python3 \
  python3-pip \
  tcptraceroute \
  ubuntu-minimal \
  unzip \
  vim-gtk \
  zip \
  zsh \
  "
  
  sudo apt-get update && sudo DEBIAN_FRONTEND=noninteractive apt-get upgrade -yq
  sudo DEBIAN_FRONTEND=noninteractive apt-get install -y ${PKGS}

}

pkgs-pip3() {

  PIP3_PACKS="flake8"

  # otherwise it complains about pip version
  pip3 install ${PIP3_PACKS}

}

manual-kubectl() {

  KUBEEXE=$(which kubectl || true)
  if [[ ${KUBEEXE} == "" ]]; then
    echo -e "${LGR}installing Kubectl${NC}"
    echo -e "${LBL}Press <Intro> when you are ready...${NC}"
    read confirm
    ${CURR_DIR}/kubectl.sh install
  else
    echo -e "${RED}Kubectl is already installed!${NC}"
    LOOP=true
    while [[ $LOOP == true ]] ; do
      read -r -n 1 -p "${1:-Do you want to REINSTALL?} [y/n]: " REPLY
      case $REPLY in
        [yY])
          echo
          echo -e "${LGR}Reinstalling Kubectl${NC}"
          ${CURR_DIR}/kubectl.sh install
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

}

manual-terraform() {
  # TODO: Move this to its own script

  # Terraform
  TFEXE=$(which terraform || true)
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
    rm terraform_0.9.6_linux_amd64.zip terraform_0.9.6_linux_386.zip terraform_0.9.6_linux_arm.zip 2>/dev/null
  else
    echo "Terraform already installed"
  fi

}

manual-ohmyzsh(){

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
          ${CURR_DIR}/ohmyzsh.sh install
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
    ${CURR_DIR}/ohmyzsh.sh install
  fi

}

manual-docker() {

  LOOP=true
  while [[ $LOOP == true ]] ; do
    read -r -n 1 -p "${1:-DOCKER, do you want to INSTALL it?} [y/n]: " REPLY
    case $REPLY in
      [yY])
        echo
        # ONLY AVAILABLE FOR ARM AND AMD64, TODO: check this is the case or skip and ALERT
        echo -e "${LGR}installing DOCKER${NC}"
        ${CURR_DIR}/docker.sh install
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

manual-awscli() {

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

pending() {

  echo "You still need to install:"
  echo "firefox developer edition"
  echo "rambox"
  echo "atom"
  echo "dropbox"

}

pkgs-aptget
pkgs-pip3
manual-kubectl
manual-terraform
manual-ohmyzsh
manual-docker
manual-awscli
pending


