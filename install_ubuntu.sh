#!/usr/bin/env bash

ARCH=$(uname -m)
GITDIR=$(pwd)


packages_cli() {
# Packages to install for cli-only systems
sudo apt-get remove --purge vim* indicator-messages

sudo apt-get install \
curl \
exfat-fuse \
exfat-utils \
expect \
fabric \
git \
iotop \
jq \
mtr \
nmap \
openssh-client \
passwd \
pwgen \
python \
python-pip \
python-boto3 \
python3 \
python3-pip \
tcptraceroute \
ubuntu-minimal \
unzip \
vim-gtk \
zip \
zsh
}

packages_x() {
# Packages to install for X
sudo apt-get install \
compizconfig-settings-manager \
keepassx \
pdftk \
terminator \
virtualbox
}

pip_packages() {
# Packages to install using pip and/or pip3
sudo pip3 install \
flake8
}

ohmyzsh(){
  echo -e "${LGR}installing ohmyszh${NC}"
  /usr/bin/xterm -e "echo 'IMPORTANT: \n when installation finishes, enter exit ON THE MAIN TERMINAL to continue'; read answer" &
  ./scripts/ohmyzsh.sh install
}

secrets() {
  # Encfs installation
  sudo apt-get update && sudo apt-get install encfs
  # Cryfs installation
  wget https://s3-eu-west-1.amazonaws.com/apt.cryfs.org/ubuntu/pool/main/c/cryfs/cryfs_0.9.7_ubuntu-16.04-x64.deb
  sudo dpkg -i --force-depends cryfs_0.9.7_ubuntu-16.04-x64.deb
  sudo apt-get -f install
  rm cryfs_0.9.7_ubuntu-16.04-x64.deb
  # Dropbox installation
  wget -O dropbox.deb https://www.dropbox.com/download?dl=packages/ubuntu/dropbox_2015.10.28_amd64.deb
  sudo dpkg -i --force-depends dropbox.deb && sudo apt-get -f install
  dropbox start -i
  # TODO: correct, add cryfs, remove encfs
  # Config keys, passwords and mountpoints
  ./scripts/secrets_config.sh install
  # mount private folder
  ./scripts/privatemount.sh
  ./scripts/privmount.sh
  # link secrets
  # TODO change to priv
  ./scripts/priv_data.sh

}

vim_config() {
  ./scripts/vim_compile.sh install
}

chrome_deb() {
# Google Chrome installation
wget https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb
sudo dpkg -i --force-depends google-chrome-stable_current_amd64.deb
sudo apt-get -f install
}

configs() {
  # terminator config
  rm ~/.config/terminator/config 2>/dev/null
  ln -s ${GITDIR}/files/terminator_config ~/.config/terminator/config
  # map CAPS lock to be another ESC
  ## This does not work on ubuntu:
  ##  rm ${HOME}.config/autostart/capstoesc.desktop 2>/dev/null
  ##  cp ${GITDIR}/files/capstoesc.desktop ${HOME}/.config/autostart/capstoesc.desktop
  sudo cp ${GITDIR}/files/capstoesc.sh /etc/profile.d/capstoesc.sh

}

awscli_pip() {
# AWSCLI installation
  pip install awscli --upgrade --user
}

kubectl_install() {
# Kubectl installation
./scripts/kubectl_config.sh
}

terraform_install() {
# Terraform installation
TFEXE=$(which terraform)
if [[ ${TFEXE} == "" ]]; then
  if test "${ARCH#*"64"}" != "$ARCH"; then
    wget https://releases.hashicorp.com/terraform/0.9.6/terraform_0.9.6_linux_amd64.zip
  elif test "${ARCH#*"86"}" != "$ARCH"; then
    wget https://releases.hashicorp.com/terraform/0.9.6/terraform_0.9.6_linux_386.zip
  elif test "${ARCH#*"arm"}" != "$ARCH"; then
    wget https://releases.hashicorp.com/terraform/0.9.6/terraform_0.9.6_linux_arm.zip
  fi
  unzip terraform_*
  sudo mv terraform /usr/local/bin/
  rm terraform_*
else
  echo "Terraform already installed"
fi
}


docker_install() {
# Docker installation
./scripts/docker.sh install
}

helm_install() {
  wget -O helm.tar.gz https://storage.googleapis.com/kubernetes-helm/helm-v2.6.1-linux-amd64.tar.gz
  tar -zxvf helm.tar.gz
  sudo mv linux-amd64/helm /usr/local/bin/helm
  rm helm.tar.gz
  rm -rf linux-amd64
  helm init --upgrade

}

kops_install() {
  wget https://github.com/kubernetes/kops/releases/download/1.7.0/kops-linux-amd64
  chmod +x kops-linux-amd64
  sudo mv kops-linux-amd64 /usr/local/bin/kops

}

rambox_install() {
  # TO BE DONE
  #wget -O rambox.tar.gz https://getrambox.herokuapp.com/download/linux_64?filetype=deb
  #mkdir -p ~/Rambox
  #TODO: problem is: how to control the directory this creates? I want Rambox instead of Rambox-05.12
  #tar -zxvf rambox.tar.gz -C ~
  #chmod +x ~/Rambox/rambox
  #ln -s /usr/local/bin/rambox
  #sudo apt-get update && sudo apt-get install libappindicator1
  echo "not yet ready"

}

atom_install() {
  wget https://github.com/atom/atom/releases/download/v1.21.0/atom-amd64.deb
  sudo dpkg --install atom-amd64.deb
  rm atom-amd64.deb
  apm install vim-mode-plus ex-mode
  rm ~/.atom/config.cson 2>/dev/null
  ln -s ${GITDIR}/files/atom_config.cson ~/.atom/config.cson

}

testing() {
# terminator cfg
#...

 echo

}

#packages_cli
#packages_x
#pip_packages
#ohmyzsh
#secrets
#vim_config
#configs
#chrome_deb
#awscli_pip
#kubectl_install
#terraform_install
#docker_install
#helm_install
#kops_install
#rambox_install
atom_install

#testing
