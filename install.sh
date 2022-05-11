#!/usr/bin/env bash

log_txt() {
  BLUE="\e[33m"
  GREEN="\e[32m"
  GREEN="\e[95m"
  RED="\e[31m"
  YELLOW="\e[34m"

  OK="\e[1;32m"
  A="\e[3;44m"
  ERROR="\e[5;31m"
  WARN="\e[1;95m"
  WARNLIST="\e[1;95m"
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
    warn)
      echo -e "${ENDCOLOR}${WARN}  ${2}${ENDCOLOR}"
      ;;
    warnlista)
      echo -e "${ENDCOLOR}${WARNLIST}  - ${2}${ENDCOLOR}"
      ;;
    warnlistb)
      echo -e "${ENDCOLOR}${WARNLIST}    - ${2}${ENDCOLOR}"
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
  bat \
  chromium-browser \
  cryfs \
  curl \
  exa \
  exfat-fuse \
  exfat-utils \
  expect \
  fabric \
  gimp \
  git \
  gnupg2 \
  htop \
  inkscape \
  iotop \
  jq \
  keepassx \
  mtr \
  net-tools \
  nmap \
  openssh-client \
  passwd \
  pcsxr \
  pwgen \
  supertuxkart \
  tcptraceroute \
  terminator \
  unzip \
  vim \
  vim-gtk \
  vlc \
  wine \
  xterm \
  zip \
  zsh
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
}

install_scripts() {
############################
  # OhMyZsh
  FUNC="Installing OH My ZSH"
  log_txt a "$FUNC"
  ./scripts/ohmyzsh.sh
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
############################
  # Rust
  # TODO: dont do this if rust is
  FUNC="Installing Rust"
  log_txt a "$FUNC"
  which rustc > /dev/null 
  if [ $? != 0 ]; then
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
    if [ $? != 0 ]; then
      log_txt aerr "$FUNC"
    else
      log_txt aok "$FUNC"
    fi
  else
    log_txt aok "$FUNC"
  fi
############################
  # Python
  FUNC="Installing Python"
  log_txt a "$FUNC"
  ./scripts/python.sh
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
############################
  # Firefox Dev Edition
  FUNC="Installing Firefox Developer Edition"
  log_txt a "$FUNC"
  ./scripts/firefox.sh
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
############################
  # Rambox
  FUNC="Installing Rambox"
  log_txt a "$FUNC"
  sudo snap install rambox
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
############################
  # Etcher
  FUNC="Installing Etcher"
  log_txt a "$FUNC"
  ./scripts/etcher.sh
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
############################
  # Arduino IDE
  FUNC="Installing Arduino IDE"
  log_txt a "$FUNC"
  sudo snap install arduino && \
  sudo usermod -a -G dialout $USER
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
############################
  # Docker
  FUNC="Installing Docker"
  log_txt a "$FUNC"
  sudo snap install docker && \
    if [ $(getent group docker) ]; then
      echo "group docker exists."
    else
      sudo groupadd docker
    fi && \
    sudo usermod -aG docker $USER
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
############################
  # MicroK8s
  FUNC="Installing MicroK8s"
  log_txt a "$FUNC"
  sudo snap install microk8s --classic
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
############################
  # Dropbox
  FUNC="Installing Dropbox"
  log_txt a "$FUNC"
  ./scripts/dropbox.sh
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
############################
  # Wine -> Trackmania
  FUNC="Installing Wine with trackmania Nations Forever"
  log_txt a "$FUNC"
  sudo snap install tmnationsforever
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
############################
  # Node + React
  FUNC="Installing NodeJS and React"
  log_txt a "$FUNC"
  curl -sL https://deb.nodesource.com/setup_14.x | sudo bash - && \
  sudo apt-get install nodejs -y && \
  sudo npm install npm@latest -g && \
  sudo npm install -g create-react-app
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
############################
  # Lunar Vim
  FUNC="Installing Lunar Vim"
  log_txt a "$FUNC"
  sudo add-apt-repository ppa:neovim-ppa/stable
  sudo apt-get update
  sudo apt-get install neovim
  bash <(curl -s https://raw.githubusercontent.com/lunarvim/lunarvim/master/utils/installer/install.sh)
  if [ $? != 0 ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
  ####
  # Steam
  # Golang
  # Virtualbox
  # VPN
  # Saml2aws
  # awscli
##    curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
##    unzip awscliv2.zip
##    sudo ./aws/install
  # terraform
  # kubectl
}

latest_considerations() {
  log_txt warn "REMEMBER TO NOW DO THE FOLLOWING:"
  log_txt warnlista "SSH Keys, GPG, credentials..."
  log_txt warnlistb "Copy them over from your backups"
  log_txt warnlista "Firefox"
  log_txt warnlistb "Import your GPG Keys to Mailvelope"
  log_txt warnlista "Rambox"
  log_txt warnlistb "Configure your accounts"
  log_txt warnlista "Dropbox"
  log_txt warnlistb "Configure your accounts"
  log_txt warnlistb "Configure autolaunch"
  log_txt warnlista "Gnome"
  log_txt warnlistb "Set CTRL-ALT-W to open Webbrowser"
  log_txt warnlistb "Set CTRL-ALT-F to open File browser"
  log_txt warnlista ".zshrc"
  log_txt warnlistb "aliases"
}

config_scripts() {

  # zshrc
  FUNC="Configuring Zshrc (exit new session to continue)"
  FUNC_OK=true
  log_txt a "$FUNC"
  if [ -f $HOME/.zshrc ]; then
    if [ $(grep "SYSANGEL VERSION" $HOME/.zshrc | wc -l) -le 0 ]; then
      mv $HOME/.zshrc $HOME/.zshrc.orig && \
      ln -s $(pwd)/files/zshrc_home $HOME/.zshrc
      if [ $? != 0 ]; then
        FUNC_OK=false
      fi
    fi
  fi
  if [[ "$FUNC_OK" != true ]]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi

  # Fonts
  FUNC="Adding new font: Fantasque"
  FUNC_OK="true"
  log_txt a "$FUNC"
  if [ ! -d /usr/share/fonts/truetype/fantasque ]; then
    sudo mkdir -p /usr/share/fonts/truetype/fantasque && \
    sudo cp files/Fantasque_Sans_Mono_Regular_Nerd_Font_Complete_Mono.ttf /usr/share/fonts/truetype/fantasque/
    if [ $? != 0 ]; then
      FUNC_OK="false"
    fi
  else
    if [ ! -f /usr/share/fonts/truetype/fantasque/FantasqueSansMono_Regular.ttf ]; then
      sudo cp files/Fantasque_Sans_Mono_Regular_Nerd_Font_Complete_Mono.ttf /usr/share/fonts/truetype/fantasque/
      if [ $? != 0 ]; then
        FUNC_OK="false"
      fi
    fi
  fi
  if [ "$FUNC_OK" != "true" ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi

  # Terminator
  FUNC="Configuring Terminator"
  FUNC_OK="true"
  log_txt a "$FUNC"
  mkdir -p $HOME/.config/terminator
  if [ ! -L $HOME/.config/terminator/config ]; then
    mv $HOME/.config/terminator/config $HOME/.config/terminator/config.orig && \
    ln -s $(pwd)/files/terminator_config ~/.config/terminator/config
    if [ $? != 0 ]; then
      FUNC_OK="false"
    fi
  fi
  if [[ "$FUNC_OK" != "true" ]]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi

  # Vim
  # Terminator
  FUNC="Configuring Vim"
  FUNC_OK="true"
  log_txt a "$FUNC"
  mkdir -p $HOME/.vim
  if [ ! -L $HOME/.vimrc ]; then
    mv $HOME/.vimrc $HOME/.vimrc.orig && \
    ln -s $(pwd)/files/vimrc $HOME/.vimrc
    if [ $? != 0 ]; then
      FUNC_OK="false"
    fi
  fi
  if [ "$FUNC_OK" != "true" ]; then
    log_txt aerr "$FUNC"
  else
    log_txt aok "$FUNC"
  fi
}

create_dirs
apt_install
install_scripts
config_scripts
latest_considerations
