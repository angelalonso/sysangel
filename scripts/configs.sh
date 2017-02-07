#!/usr/bin/env bash
# Installs all parts required for the private mounpoint to work automatically
USR=$(whoami)
HOME="/home/${USR}"
GITDIR="${HOME}/sysangel"

install(){
  echo "installing fonts"
  cd ${HOME}
  git clone https://github.com/powerline/fonts
  cd fonts
  git clone https://github.com/belluzj/fantasque-sans
  ./install.sh

  echo "installing terminator config"
  mkdir -p ${HOME}/.config/terminator
  cp ${GITDIR}/files/terminator_config ${HOME}/.config/terminator/config


}

remove(){
  echo "uninstalling"
  #TODO: this
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
