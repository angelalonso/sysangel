#!/usr/bin/env bash

INSTALLDIR="~/.sysangel"

UBUNTUPKGS="python git"
DEBIANPKGS="python git"

dependencies(){
  # Install all required packages for sysangel.py
  ./scripts/packages.sh install $@
  #sudo apt-get update && sudo apt-get install $1
}

main(){
  # Find out the current distro
  SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)

  case "${SYSTEM}" in
    Ubuntu|ubuntu)
      dependencies "${UBUNTUPKGS}"
      ;;
    Debian|debian)
      dependencies "${DEBIANPKGS}"
      ;;
  esac

  # Create directories for the installation
  mkdir -p ${INSTALLDIR}/TMP

  ./sysangel.py install "${SYSTEM}"

  # Remove directories that were created for the installation
  rm -r ${INSTALLDIR}/TMP

}
main
