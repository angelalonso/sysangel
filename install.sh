#!/usr/bin/env bash

log_txt() {
  BLUE="\e[33m"
  GREEN="\e[32m"
  RED="\e[31m"
  YELLOW="\e[34m"

  OK="\e[1;32m"
  A="\e[3;44m"
  ERROR="\e[5;31m"
  ENDCOLOR="\e[0m"

  case $1 in
    a)
      echo -e "${ENDCOLOR}${A}- ${2}${ENDCOLOR}"
      ;;
    aok)
      echo -e "${ENDCOLOR}${OK}  ${2} done.${ENDCOLOR}"
      ;;
    aerr)
      echo -e "${ENDCOLOR}${ERROR}  Err. at ${2}${ENDCOLOR}"
      exit 2
      ;;
    b)
      echo -e "${ENDCOLOR}  - ${2}${ENDCOLOR}"
      ;;
    *)
      echo "${2}"
      ;;
  esac
}

create_dirs() {
  FUNC="Creating directories"
  log_txt a "$FUNC"
  mkdir -p ~/Software/Dev && \
  mkdir -p ~/Software/Work && \
  mkdir -p ~/.ssh && \
  mkdir -p ~/.aws && \
  mkdir -p ~/.kube
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
}

apt_install() {
  FUNC="Installing stuff through Apt"
  log_txt a "$FUNC"
  sudo apt-get update && \
  sudo apt-get upgrade && \
  sudo apt-get install -y \
  apt-transport-https \
  arp-scan \
  chromium-browser \
  cryfs \
  curl \
  gimp \
  git \
  htop \
  inkscape \
  iotop \
  jq \
  keepassx \
  mtr \
  net-tools \
  nmap \
  pwgen \
  terminator \
  unzip \
  vim \
  vlc \
  zip \
  zsh
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
}

create_dirs
apt_install
# install_scripts
# config_scripts
#   vimrc
#   terminator config

