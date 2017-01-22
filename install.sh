#!/usr/bin/env bash

INSTALLDIR="~/.sysangel"
TMPDIR="${INSTALLDIR}/tmp"
KEYSDIR="${INSTALLDIR}/keys"

UBUNTUPKGS="encfs git passwd python"
DEBIANPKGS="encfs git passwd python"

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

  # Create directories needed for the future
  mkdir -p ${INSTALLDIR}
  mkdir -p ${KEYSDIR}
  # Create directories only for the installation
  mkdir -p ${TMPDIR}

  ./sysangel.py install "${SYSTEM}"

  # Remove directories that were created for the installation
  rm -r ${TMPDIR}

}
main
