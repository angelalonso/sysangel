#!/usr/bin/env bash

USR=$(whoami)
INSTALLDIR="${HOME}/.sysangel"
TMPDIR="${INSTALLDIR}/tmp"
KEYSDIR="${INSTALLDIR}/keys"


PKGS="encfs expect git keepassx passwd pwgen python sudo terminator \
zim zsh"

UBUNTUPKGS="${PKGS}"
DEBIANPKGS="${PKGS}"

dependencies(){
  # Install all required packages for sysangel.py
  ./scripts/packages.sh install $@
  #sudo apt-get update && sudo apt-get install $1
}

main(){
  # Find out the current distro
  SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)
  echo "System is ${SYSTEM}"

  case "${SYSTEM}" in
    Ubuntu|ubuntu)
      dependencies "${UBUNTUPKGS}"
      ;;
    Debian|debian)
      dependencies "${DEBIANPKGS}"
      ;;
  esac

  # Add user to sudoers
  if [ $(su - root -c 'grep aaf /etc/sudoers | wc -l') -lt 1 ]; then
    su - root -c 'echo "aaf ALL=(ALL:ALL) ALL" >> /etc/sudoers'
  fi

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
