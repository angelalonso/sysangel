#!/usr/bin/env bash

UBUNTUPKGS="python"
DEBIANPKGS="python"

dependencies(){
  # Install all required packages for sysangel.py
  echo $1
  #sudo apt-get update && sudo apt-get install $1
}

main(){
  # Find out the current distro
  SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)

  case "${SYSTEM}" in
    Ubuntu)
      dependencies ${UBUNTUPKGS}
      ;;
    Debian)
      dependencies ${DEBIANPKGS}
      ;;
  esac

  ./sysangel.py install "${SYSTEM}"
}
main
