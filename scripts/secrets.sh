#!/usr/bin/env bash

# Installs all parts required for the private mounpoint to work automatically

INSTALLDIR="${HOME}/.sysangel"
TMPDIR="${INSTALLDIR}/tmp"
SCRIPTSDIR="${INSTALLDIR}/scripts"
KEYSDIR="${INSTALLDIR}/keys"

SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)

install_dropbox(){
  echo "installing structure"
  SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)
  UNAME_M=$(uname -m)
  case ${UNAME_M} in
    i686)
      ARCH="x86";;
    x86_64)
      ARCH="x86_64";;
    *)
      ARCH="x86";;
  esac

  # First up, get Dropbox installed
  #  based on https://www.dropbox.com/install-linux
  echo "#### GET READY! \n
  We will install DROPBOX, so you should go look for your user and password RIGHT NOW!"

  # TODO: only download if file is not yet there
  cd ${HOME} && wget -O - "https://www.dropbox.com/download?plat=lnx.$ARCH" | tar xzf -

  ${HOME}/.dropbox-dist/dropboxd &
}

remove_dropbox(){
  echo "cleaning up structure"

  case "${SYSTEM}" in
    ubuntu|Ubuntu)
      sudo killall dropbox || sudo killall dropboxd
      ;;
    debian|Debian)
      su - root -c "killall dropbox || killall dropboxd"
      ;;
  esac

  rm -r ${HOME}/.dropbox-dist

}

install_encfs(){
  echo "configuring encfs mountpoint"

  echo "generating bridge keys"
  openssl genrsa -out ${KEYSDIR}/priv.key 4096
  openssl rsa -in ${KEYSDIR}/priv.key -pubout > ${KEYSDIR}/pub.key

  # http://stackoverflow.com/questions/1923435/how-do-i-echo-stars-when-reading-password-with-read
  prompt="Enter Password:"
  while IFS= read -p "$prompt" -r -s -n 1 char
  do
      if [[ $char == $'\0' ]]
      then
          break
      fi
      prompt='*'
      password+="$char"
  done
  echo "${password}" | openssl rsautl -inkey  ${KEYSDIR}/pub.key -pubin -encrypt >  ${KEYSDIR}/main_encfs.pass

  case "${SYSTEM}" in
    ubuntu|Ubuntu)
      sudo cp ${SCRIPTSDIR}/profile_encfs.sh /etc/profile.d/privatemount.sh && \
        sudo chown root:root /etc/profile.d/privatemount.sh && \
        sudo chmod 644 /etc/profile.d/privatemount.sh
      ;;
    debian|Debian)
      su - root -c "cp ${SCRIPTSDIR}/profile_encfs.sh /etc/profile.d/privatemount.sh && \
        chown root:root /etc/profile.d/privatemount.sh && \
        chmod 644 /etc/profile.d/privatemount.sh"
      ;;
  esac

}

remove_encfs(){
  echo "removing encfs mountpoint"
  rm ${KEYSDIR}/priv.key
  rm ${KEYSDIR}/pub.key
  rm ${KEYSDIR}/main_encfs.pass

  sudo rm /etc/profile.d/privatemount.sh
  case "${SYSTEM}" in
    ubuntu|Ubuntu)
      sudo rm /etc/profile.d/privatemount.sh
      ;;
    debian|Debian)
      su - root -c "rm /etc/profile.d/privatemount.sh"
      ;;
  esac

}

case "$1" in
  install|i|Install|I)
    install_dropbox
    install_encfs
    ;;
  remove|Remove|r|R|uninstall|u|Uninstall|U)
    remove_dropbox
    remove_encfs
    ;;
  *)
    echo "ERROR: Syntax is $0 [install|remove|]"
    ;;
esac
