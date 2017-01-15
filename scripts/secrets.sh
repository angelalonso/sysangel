#!/usr/bin/env bash
# Installs all parts required for the private mounpoint to work automatically

install(){
  echo "installing structure"
}

remove(){
  echo "uninstalling structure"
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
