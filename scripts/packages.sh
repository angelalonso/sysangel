#!/usr/bin/env bash
# Installs all parts required for the private mounpoint to work automatically

install(){
  PKGS=$(echo $@ | cut -d' ' -f2-)
  SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)

  echo "Installing ${PKGS}"

  case "${SYSTEM}" in
    ubuntu|Ubuntu)
      sudo apt-get update && sudo apt-get install ${PKGS}
      ;;
    debian|Debian)
      su - root -c "apt-get update && apt-get install ${PKGS}"
      ;;
  esac
}

remove(){
  PKGS=$(echo $@ | cut -d' ' -f2-)
  SYSTEM=$(grep "^ID=" /etc/*-release | cut -d '=' -f 2)

  echo "Removing ${PKGS}"

  case "${SYSTEM}" in
    ubuntu|Ubuntu)
      sudo apt-get remove --purge ${PKGS}
      ;;
    debian|Debian)
      su - root -c "apt-get remove --purge ${PKGS}"
      ;;
  esac
}

echo "number of params is"
echo $#
if [[ $# -lt 2 ]]; then
  echo "Illegal number of parameters"
  exit 2
fi

case "$1" in
  install|i|Install|I)
    install $@
    ;;
  remove|Remove|r|R|uninstall|u|Uninstall|U)
    remove $@
    ;;
  *)
    echo "ERROR: Syntax is $0 [install|remove|] <packages>"
    ;;
esac
