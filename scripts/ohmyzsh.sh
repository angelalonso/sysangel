#!/usr/bin/env bash
# Installs all parts required for the private mounpoint to work automatically

install(){
  echo "installing"
  sh -c "$(wget https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh -O -)"
}

remove(){
  echo "uninstalling"
  uninstall_oh_my_zsh
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
