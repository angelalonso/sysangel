#!/bin/usr/env bash
#
# Script to configure a new machine to use sysangel
set -eo pipefail

INSTALLDIR="$HOME/sysangel"
PROFILEDIR="/etc/profile.d"

main() {
  echo "- Installing work directory..."
  mkdir -p ${INSTALLDIR} &> /dev/null
  echo "  DONE"

  echo "- Installing Profile.d script..."
  curl -o -s /etc/profile.d/sysangel.sh \
    https://raw.githubusercontent.com/angelalonso/sysangel/master/profile_sysangel.sh \
    &> /dev/null
  echo "  DONE"

  echo "- Installing ${INSTALLDIR} main python script..."
  curl -o -s ${INSTALLDIR}/sysangel.py \
    https://raw.githubusercontent.com/angelalonso/sysangel/master/sysangel.py \
    &> /dev/null
  echo "  DONE"

  echo "- Please, indicate name for this machine"

}

undo() {
  rmdir -p $HOME/sysangel
  echo "- Installation dir removed"

}

main
#undo
