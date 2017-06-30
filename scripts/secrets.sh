#!/usr/bin/env bash

# Installs all parts required for the private mounpoint to work automatically

USR=$(whoami)
HOME="/home/${USR}"
INSTALLDIR="${HOME}/.sysangel"
TMPDIR="${INSTALLDIR}/tmp"
KEYSDIR="${INSTALLDIR}/keys"
GITDIR="${HOME}/sysangel"
SCRIPTSDIR="${GITDIR}/scripts"
FILESDIR="${GITDIR}/files"

SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)

install_dropbox(){
  echo "installing structure"
  SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)
  UNAME_M=$(uname -m)
  case ${UNAME_M} in
    x86_64)
      ARCH="amd_64";;
    *)
      ARCH="i386";;
  esac

  # First up, get Dropbox installed
  #  based on https://www.dropbox.com/install-linux
  echo "#### GET READY! \n
  We will install DROPBOX, so you should go look for your user and password RIGHT NOW!"

  cd ${HOME} && wget https://www.dropbox.com/download?dl=packages/debian/dropbox_2015.10.28_$ARCH.deb
  sudo dpkg -i dropbox_2015.10.28_$ARCH.deb
  dropbox start -i
  ## OLD METHOD
  ## # TODO: only download if file is not yet there
  ## cd ${HOME} && wget -O - "https://www.dropbox.com/download?plat=lnx.$ARCH" | tar xzf -
  ## ${HOME}/.dropbox-dist/dropboxd &
}

remove_dropbox(){
  echo "cleaning up structure"

  case "${SYSTEM}" in
    ubuntu|Ubuntu|debian|Debian)
      sudo killall dropbox; sudo killall dropboxd
      ;;
    *)
      echo "SYSTEM NOT YET SUPPORTED"
      ;;
  esac

  rm -r ${HOME}/.dropbox-dist 2>/dev/null
  sudo apt remove --purge dropbox

}

install_encfs(){
  echo "configuring encfs mountpoint"

  echo "generating bridge keys"
  openssl genrsa -out ${KEYSDIR}/priv.key 4096
  openssl rsa -in ${KEYSDIR}/priv.key -pubout > ${KEYSDIR}/pub.key

  # http://stackoverflow.com/questions/1923435/how-do-i-echo-stars-when-reading-password-with-read
  # TODO: hide answer
  echo "Enter Password for Encfs:"
  read password
  echo "${password}" | openssl rsautl -inkey  ${KEYSDIR}/pub.key -pubin -encrypt >  ${KEYSDIR}/main_encfs.pass

  case "${SYSTEM}" in
    ubuntu|Ubuntu|debian|Debian)
      mkdir -p ${HOME}/.config/autostart
      ln -s ${HOME}/sysangel/files/private_mount.desktop ${HOME}/.config/autostart/private_mount.desktop
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

  case "${SYSTEM}" in
    ubuntu|Ubuntu|debian|Debian)
      sudo rm /etc/profile.d/privatemount.sh
      ;;
    *)
      echo "SYSTEM NOT YET SUPPORTED"
      ;;
  esac

}

case "$1" in
  install|i|Install|I)
    install_encfs
    install_dropbox
    ;;
  remove|Remove|r|R|uninstall|u|Uninstall|U)
    remove_dropbox
    remove_encfs
    ;;
  *)
    echo "ERROR: Syntax is $0 [install|remove|]"
    ;;
esac
