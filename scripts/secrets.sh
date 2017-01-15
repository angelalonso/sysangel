#!/usr/bin/env bash
# Installs all parts required for the private mounpoint to work automatically

install(){
  echo "installing structure"
  SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)
  ARCH=$(uname -m)

  cd ~ & wget -O - "https://www.dropbox.com/download?plat=lnx.$ARCH" | tar xzf -

  ~/.dropbox-dist/dropboxd &
}

remove(){
  echo "cleaning up structure"
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
