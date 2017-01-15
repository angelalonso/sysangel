#!/usr/bin/env bash

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

  ./sysangel.py install "${SYSTEM}"
}
main
