#!/usr/bin/env bash

# Installs all parts required for the private mounpoint to work automatically

USR=$(whoami)
HOME="/home/${USR}"
INSTALLDIR="${HOME}/.sysangel"
TMPDIR="${INSTALLDIR}/tmp"
KEYSDIR="${INSTALLDIR}/keys"
GITDIR="${HOME}/Software/Dev/sysangel"
SCRIPTSDIR="${GITDIR}/scripts"
FILESDIR="${GITDIR}/files"


# Bash colors
BLU='\033[0;34m'
LGR='\033[1;32m'
LBL='\033[1;34m'
ORN='\033[0;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

install_encfs(){
  echo "configuring encfs mountpoint"
  mkdir -p $HOME/Private
  echo "configuring cryfs mountpoint"
  mkdir -p $HOME/Priv

  echo "generating bridge keys"
  mkdir -p ${KEYSDIR}
  openssl genrsa -out ${KEYSDIR}/priv.key 4096
  openssl rsa -in ${KEYSDIR}/priv.key -pubout > ${KEYSDIR}/pub.key

  echo "Enter Password for Encfs:"
  # http://stackoverflow.com/questions/1923435/how-do-i-echo-stars-when-reading-password-with-read#1923503
  while IFS= read -p "$prompt" -r -s -n 1 char
  do
      # Enter - accept password
      if [[ $char == $'\0' ]] ; then
          break
      fi
      # Backspace
      if [[ $char == $'\177' ]] ; then
          prompt=$'\b \b'
          password="${password%?}"
      else
          prompt='*'
          password+="$char"
      fi
  done
  echo
  echo "${password}" | openssl rsautl -inkey  ${KEYSDIR}/pub.key -pubin -encrypt >  ${KEYSDIR}/main_encfs.pass

  echo "Enter Password for Cryfs:"
  # http://stackoverflow.com/questions/1923435/how-do-i-echo-stars-when-reading-password-with-read#1923503
  while IFS= read -p "$prompt" -r -s -n 1 char
  do
      # Enter - accept password
      if [[ $char == $'\0' ]] ; then
          break
      fi
      # Backspace
      if [[ $char == $'\177' ]] ; then
          prompt=$'\b \b'
          pass="${pass%?}"
      else
          prompt='*'
          pass+="$char"
      fi
  done
  echo
  echo "${pass}" | openssl rsautl -inkey  ${KEYSDIR}/pub.key -pubin -encrypt >  ${KEYSDIR}/cryfs.pass

  case "${DIST}" in
    ubuntu|Ubuntu|debian|Debian)
      # ENCFS
      sudo -i env DIR="${SCRIPTSDIR}" sh -c 'cat ${DIR}/xfce_profile_encfs.sh >> /etc/profile.d/privatemount.sh' && \
        sudo chown root:root /etc/profile.d/privatemount.sh && \
        sudo chmod 755 /etc/profile.d/privatemount.sh
      # CRYFS
      sudo -i env DIR="${SCRIPTSDIR}" sh -c 'cat ${DIR}/profile_cryfs.sh >> /etc/profile.d/privmount.sh' && \
        sudo chown root:root /etc/profile.d/privmount.sh && \
        sudo chmod 755 /etc/profile.d/privmount.sh
      #TODO:
      # Automate mount after dropbox got all data
      # Create a mount/unmount script/button
      ;;
    *)
      echo "SYSTEM NOT YET SUPPORTED"
      ;;
  esac

}

remove_encfs(){
  echo "removing encfs mountpoint"
  rm ${KEYSDIR}/priv.key
  rm ${KEYSDIR}/pub.key
  rm ${KEYSDIR}/main_encfs.pass

  case "${DIST}" in
    ubuntu|Ubuntu|debian|Debian)
      sudo rm /etc/profile.d/privatemount.sh
      ;;
    *)
      echo "SYSTEM NOT YET SUPPORTED"
      ;;
  esac

}

# Find out the distro you are on
if [[ $(uname -a) == *"Debian"* ]]; then
  DIST="Debian"
elif [[ $(uname -a) == *"buntu"* ]]; then
  DIST="Ubuntu"
else
  DIST="Other"
fi

case "$1" in
  install|i|Install|I)
    KEYS_EXIST=$(ls ${KEYSDIR} 2>/dev/null | grep "main_encfs.pass\|priv.key\|pub.key" | wc -l)
    if [[ "${KEYS_EXIST}" -gt 2 ]]; then
      echo -e "${RED}Keys already exist!!${NC}"
      LOOP=true
      while [[ $LOOP == true ]] ; do
        read -r -n 1 -p "${1:-Do you want to RECONFIGURE them?} [y/n]: " REPLY
        case $REPLY in
          [yY])
            echo
            echo -e "${LGR}reconfiguring Keys${NC}"
            install_encfs
            LOOP=false
            ;;
          [nN])
            echo
            LOOP=false
            ;;
          *) printf " \033[31m %s \n\033[0m" "invalid input"
        esac
      done
    else
      echo -e "${LGR}reconfiguring Keys${NC}"
      install_encfs
    fi
    ;;
  remove|Remove|r|R|uninstall|u|Uninstall|U)
    remove_dropbox
    remove_encfs
    ;;
  *)
    echo "ERROR: Syntax is $0 [install|remove|]"
    ;;
esac
