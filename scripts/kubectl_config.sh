#!/usr/bin/env bash
# Installs Kubectl


install(){
  echo "installing Kubectl"
  if [ "$(uname -m)" == "i686" ]; then
    VERS="386"
  else
    VERS="amd64"
  fi

  echo $VERS
  curl -LO https://storage.googleapis.com/kubernetes-release/release/$(curl -s https://storage.googleapis.com/kubernetes-release/release/stable.txt)/bin/linux/$VERS/kubectl

  chmod +x ./kubectl
  sudo mv ./kubectl /usr/local/bin/kubectl
  exit
}

remove(){
  echo "uninstalling Kubectl"
  sudo rm /usr/local/bin/kubectl
}

case "$1" in
  install|i|Install|I)
    install
    ;;
  remove|Remove|r|R|uninstall|u|Uninstall|U)
    remove
    ;;
  *)
    echo "ERROR: Syntax is $0 [install|remove|]"
    ;;
esac

