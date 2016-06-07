#!/bin/usr/env bash
#
# Script to configure a new machine to use sysangel
set -eo pipefail

INSTALLDIR="$HOME/sysangel"
PROFILEDIR="/etc/profile.d"
HOSTNAME=$(hostname)

PYTHON="/usr/bin/python3"

configfile() {

  read -p "- Please, indicate name for this machine ["${HOSTNAME}"] " answer
  if [ "${answer}" = "" ]; then MACHINE=${HOSTNAME}
  else MACHINE=${answer}; fi
  echo "From now on we will refer to this machine as ${MACHINE}"

  # Add roles for this machine
  echo "# Configuration roles for this machine, feel free to modify" > ${INSTALLDIR}/${MACHINE}.roles 
  echo "#   But bear in mind they will be used in strict order, last overwrites previous" >> ${INSTALLDIR}/${MACHINE}.roles 
  echo "ROLES:" >> ${INSTALLDIR}/${MACHINE}.roles
  echo "  - "${MACHINE} >> ${INSTALLDIR}/${MACHINE}.roles
  echo "  - common" >> ${INSTALLDIR}/${MACHINE}.roles
  
  # Add some characteristics of this machine
  echo "# Attention! These Facts are not meant to be changed manually" >> ${INSTALLDIR}/${MACHINE}.roles
  echo "FACTS:" >> ${INSTALLDIR}/${MACHINE}.roles
  export PYTHONHOME=/usr/
  ${PYTHON} ${INSTALLDIR}/sysangel.py get-distro

}

main() {
  echo "- Installing work directory..."
  mkdir -p ${INSTALLDIR} &> /dev/null
  echo "  DONE"

  echo "- Installing Profile.d script..."
  sudo wget -O /etc/profile.d/sysangel.sh \
    https://raw.githubusercontent.com/angelalonso/sysangel/master/profile_sysangel.sh \
    &> /dev/null
  echo "  DONE"

  echo "- Installing ${INSTALLDIR} main python script..."
  wget -O ${INSTALLDIR}/sysangel.py \
    https://raw.githubusercontent.com/angelalonso/sysangel/master/sysangel.py \
    &> /dev/null
  wget -O ${INSTALLDIR}/yaml \
    https://raw.githubusercontent.com/angelalonso/sysangel/master/yaml \
    &> /dev/null
  echo "  DONE"
  wget -O ${INSTALLDIR}/platform \
    https://raw.githubusercontent.com/angelalonso/sysangel/master/platform \
    &> /dev/null

  configfile
  
}

undo() {
  rmdir -p $HOME/sysangel
  echo "- Installation dir removed"

}

main
#undo
