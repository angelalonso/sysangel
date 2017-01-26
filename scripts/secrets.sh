#!/usr/bin/env bash

# Installs all parts required for the private mounpoint to work automatically

USR=$(whoami)
HOME="/home/${USR}"
INSTALLDIR="${HOME}/.sysangel"
TMPDIR="${INSTALLDIR}/tmp"
KEYSDIR="${INSTALLDIR}/keys"
GITDIR="${HOME}/sysangel"
SCRIPTSDIR="${GITDIR}/scripts"

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
      sudo killall dropbox; sudo killall dropboxd
      ;;
    debian|Debian)
      echo "killing dropbox"
      su - root -c "killall dropbox; killall dropboxd"
      echo "killed dropbox"
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
  echo "Enter Password for Encfs:"
  read password
  echo "${password}" | openssl rsautl -inkey  ${KEYSDIR}/pub.key -pubin -encrypt >  ${KEYSDIR}/main_encfs.pass

  case "${SYSTEM}" in
    ubuntu|Ubuntu|debian|Debian)
      sudo sh -c 'echo "#!/usr/bin/env bash" > /etc/profile.d/privatemount.sh' && \
        sudo -i env USR="$USR" sh -c 'echo "USER=${USR}" >> /etc/profile.d/privatemount.sh' && \
        sudo -i env DIR="${SCRIPTSDIR}" sh -c 'cat ${DIR}/profile_encfs.sh >> /etc/profile.d/privatemount.sh' && \
        sudo chown root:root /etc/profile.d/privatemount.sh && \
        sudo chmod 644 /etc/profile.d/privatemount.sh
      ;;
    *)
      echo "HERE  -> "$SCRIPTSDIR ${SCRIPTSDIR}
      echo "############################################"
      su - root -c 'echo "HERE  -> "$SCRIPTSDIR ${SCRIPTSDIR}'
      echo "############################################"
      su - root -c 'echo "#!/usr/bin/env bash" > /etc/profile.d/privatemount.sh && \
        echo "USER="$USR >> /etc/profile.d/privatemount.sh && \
	echo "HERE  -> "$SCRIPTSDIR ${SCRIPTSDIR}
        cat $SCRIPTSDIR/profile_encfs.sh >> /etc/profile.d/privatemount.sh && \
        chown root:root /etc/profile.d/privatemount.sh && \
        chmod 644 /etc/profile.d/privatemount.sh'
      echo "############################################"

      ;;
  esac

}

remove_encfs(){
  echo "removing encfs mountpoint"
  rm ${KEYSDIR}/priv.key
  rm ${KEYSDIR}/pub.key
  rm ${KEYSDIR}/main_encfs.pass

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
