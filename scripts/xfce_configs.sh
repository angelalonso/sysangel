#!/usr/bin/env bash


USR=$(whoami)
HOME="/home/${USR}"
INSTALLDIR="${HOME}/.sysangel"
TMPDIR="${INSTALLDIR}/tmp"
KEYSDIR="${INSTALLDIR}/keys"
GITDIR="${HOME}/sysangel"
FILESDIR="${GITDIR}/files"


# Bash colors
BLU='\033[0;34m'
LGR='\033[1;32m'
LBL='\033[1;34m'
ORN='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color


install(){

  theme
# We dont need Elementary -> Faenza installed as package
#  icons
  fonts
  terminator
  zsh
  autostart
  keymaps
  xmlconfigs

}


remove(){
  theme_remove
  echo "uninstalling not yet ready"
}

# Install theme
theme(){

  echo -e "${LGR}installing theme${NC}"
  echo -e "${LBL}Press <Intro> when you are ready...${NC}"
  read confirm
  mkdir -p ~/.themes
  cd $_
  wget https://github.com/shimmerproject/Greybird/archive/master.zip
  unzip master.zip
  rm master.zip

}


# Remove theme
theme_remove(){

  echo -e "${LGR}removing theme${NC}"
  echo -e "${LBL}Press <Intro> when you are ready...${NC}"
  read confirm
  rm ~/.themes/Greybird-master 2>/dev/null

}


# Install Icons
icons(){

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

}


# Install Fonts
fonts(){

  echo -e "${LGR}installing fonts${NC}"
  # Thanks to https://github.com/belluzj/fantasque-sans
  # I already compiled my own, I'd rather just copy it
  sudo mkdir -p /usr/local/share/fonts/f
  sudo cp ${GITDIR}/files/FantasqueSansMono_Regular.ttf /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf
  sudo chmod 644 /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf
  sudo chown root:staff /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf

}


# Install Terminator config
terminator(){

  echo -e "${LGR}installing terminator config${NC}"
  mkdir -p ${HOME}/.config/terminator

  if [[ ! -f ${HOME}/.config/terminator/config.orig ]]; then
    mv ${HOME}/.config/terminator/config ${HOME}/.config/terminator/config.orig 2>/dev/null
  else
    rm ${HOME}/.config/terminator/config 2>/dev/null
  fi
  ln -s ${HOME}/Dropbox/data/config_open/terminator_config ${HOME}/.config/terminator/config

}


# Install ZSH config
zsh(){

  echo -e "${LGR}installing ZSH config ${NC}"

  if [[ ! -f ${HOME}/.zshrc.orig ]]; then
    mv ${HOME}/.zshrc ${HOME}/.zshrc.orig 2>/dev/null
  else
    rm -rf ${HOME}/.zshrc 2>/dev/null
  fi
  ln -s ${GITDIR}/files/zshrc_home ${HOME}/.zshrc

}


# Autostart applications .desktop files
autostart(){

  if [[ ! -f ${HOME}/.config/autostart ]]; then
    mv ${HOME}/.config/autostart ${HOME}/.config/autostart.orig 2>/dev/null
  else
    rm -rf ${HOME}/.config/autostart 2>/dev/null
  fi
  ln -s ${GITDIR}/files/xfce_autostart ${HOME}/.config/autostart

}


# Key mapping tools and configs
keymaps(){

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

}


# xfce xml config files
xmlconfigs(){

  if [[ ! -f ${HOME}/.config/xfce4 ]]; then
    mv ${HOME}/.config/xfce4 ${HOME}/.config/xfce4.orig 2>/dev/null
  else
    rm -rf ${HOME}/.config/xfce4 2>/dev/null
  fi
  ln -s ${GITDIR}/files/xfce/xfce4 ${HOME}/.config/xfce4

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
