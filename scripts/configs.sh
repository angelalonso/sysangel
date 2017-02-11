#!/usr/bin/env bash
# Installs all parts required for the private mounpoint to work automatically
USR=$(whoami)
HOME="/home/${USR}"
GITDIR="${HOME}/sysangel"

install(){
  echo "installing fonts"
  # Thanks to https://github.com/belluzj/fantasque-sans
  # I already compiled my own, I'd rather just copy it
  sudo mkdir -p /usr/local/share/fonts/f

  sudo cp ${GITDIR}/files/FantasqueSansMono_Regular.ttf /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf

  sudo chmod 644 /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf
  sudo chown root:staff /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf

  #TODO: backup if there is no previous backup

  echo "installing terminator config"
  mkdir -p ${HOME}/.config/terminator
  cp ${GITDIR}/files/terminator_config ${HOME}/.config/terminator/config


}

remove(){
  echo "uninstalling"
  #TODO: restore from backup
  sudo rm /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf
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
