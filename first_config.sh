#!/bin/usr/env bash
#
# Script to configure a new machine to use sysangel

INSTALLDIR="$HOME/sysangel"
PROFILEDIR="/etc/profile.d"

main() {
  echo "- Installing work directory..."
  mkdir -p ${INSTALLDIR}
  echo "  DONE"

  echo "- Installing Profile.d script..."
  curl -o /etc/profile.d/sysangel.sh https://raw.githubusercontent.com/angelalonso/sysangel/master/profile_sysangel.sh
  echo "  DONE"

  echo " - Installing ${INSTALLDIR} main python script..."
  curl -o ${INSTALLDIR}/sysangel.py https://raw.githubusercontent.com/angelalonso/sysangel/master/sysangel.py
  echo "  DONE"

}

undo() {
  rmdir -p $HOME/sysangel
  echo "- Installation dir removed"

}

main
#undo
