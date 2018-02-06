#!/usr/bin/env bash
# Installs OhMyZsh


install(){
  echo "installing OhMyZsh"
  sh -c "$(wget https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh -O -)"
  exit
}

remove(){
  echo "uninstalling OhMyZsh"
  bash ${HOME}/.oh-my-zsh/tools/uninstall.sh
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
