#!/bin/usr/env bash
#
# Script to configure a new machine to use sysangel

INSTALLDIR="$HOME/sysangel"
PROFILEDIR="/etc/profile.d"

main() {
  mkdir -p ${INSTALLDIR}
  echo "- Installation dir created"

  curl -o /etc/profile.d/sysangel.sh https://raw.githubusercontent.com/angelalonso/sysangel/master/profile_sysangel.sh
  echo "- Profile.d script installed"

  curl -o ${INSTALLDIR}/sysangel.py https://raw.githubusercontent.com/angelalonso/sysangel/master/sysangel.py
  echo "- Profile.d script installed"

}

undo() {
  rmdir -p $HOME/sysangel
  echo "- Installation dir removed"

}

main
#undo
