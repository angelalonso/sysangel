#!/usr/bin/env bash
# Installs all parts required for the private mounpoint to work automatically

install(){
  echo "installing structure"
  SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)
  UNAME_M=$(uname -m)
  case ${UNAME_M} in
    i686)
      ARCH="x86";;
    x86_64)
      ARCH="x86_64";;
    *)
      ARCH="x86";;
  esac

  # First up, get Dropbox installed
  #  based on https://www.dropbox.com/install-linux
  echo "#### GET READY! \n
  We will install DROPBOX, so you should go look for your user and password RIGHT NOW!"

  cd ~ && wget -O - "https://www.dropbox.com/download?plat=lnx.$ARCH" | tar xzf -
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
