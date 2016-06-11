#!/bin/usr/env bash
#
# Script to configure a new machine to use sysangel
set -eo pipefail

INSTALLDIR="${HOME}/sysangel"
ROLEDIR="${INSTALLDIR}/ROLES"
PROFILEDIR="/etc/profile.d"
HOSTNAME=$(hostname)

PYTHON="/usr/bin/python"

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

  # Download the definitions for the first time
  echo "- Downloading definition for 'common'"
  wget -O ${ROLEDIR}/common.yaml \
          https://raw.githubusercontent.com/angelalonso/sysangel/master/ROLES/common.yaml \
              &> /dev/null
  echo "  DONE"
  # For this machine's one we have to be more careful
  echo "- Creating definition for '${MACHINE}'"
  if [ -e "${ROLEDIR}/${MACHINE}.yaml" ]; then
    echo "${ROLEDIR}/${MACHINE}.yaml exists"
    while true; do
      read -p "Do you want to overwrite? " answer
      case $answer in
        [yY]* ) echo "INSTALL:" > ${ROLEDIR}/${MACHINE}.yaml
          echo >> ${ROLEDIR}/${MACHINE}.yaml
          echo "SPECIAL_INSTALL:" >> ${ROLEDIR}/${MACHINE}.yaml
          echo "  DONE"
          break;;
        [nN]* ) exit;;
        * )     echo "Dude, just enter Y or N, please.";;
      esac
    done
  else
    echo "INSTALL:" >> ${ROLEDIR}/${MACHINE}.yaml
    echo >> ${ROLEDIR}/${MACHINE}.yaml
    echo "SPECIAL_INSTALL:" >> ${ROLEDIR}/${MACHINE}.yaml
    echo "  DONE"
  fi


  
  # Add some characteristics of this machine
  echo "# Attention! These Facts are not meant to be changed manually" >> ${INSTALLDIR}/${MACHINE}.roles
  echo "FACTS:" >> ${INSTALLDIR}/${MACHINE}.roles
  # First the Distro and codename
  DISTRO=$(${PYTHON} ${INSTALLDIR}/sysangel.py get-distro)
  DIST=$(echo ${DISTRO} | awk -F "," '/1/ {print $1}')
  #VERSION=$(echo ${DISTRO} | awk -F "," '/1/ {print $2}')
  CODENAME=$(echo ${DISTRO} | awk -F "," '/1/ {print $3}')
  echo "  distro:" >> ${INSTALLDIR}/${MACHINE}.roles
  echo "    "${DIST} >> ${INSTALLDIR}/${MACHINE}.roles
  echo "  codename:" >> ${INSTALLDIR}/${MACHINE}.roles
  echo "    "${CODENAME} >> ${INSTALLDIR}/${MACHINE}.roles
}

presentation() {
  echo "####              ####"
  echo "#### {\SYSANGEL/} ####"
  echo "#### Installation ####"
  echo "######################"
  echo
}

main() {
  presentation
  echo "- Installing work directory..."
  mkdir -p ${ROLEDIR} &> /dev/null
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
  echo "  DONE"

  configfile
  
}

undo() {
  rmdir -p $HOME/sysangel
  echo "- Installation dir removed"

}

main
#undo
