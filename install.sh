#!/bin/bash

#I had to 
# partition from windows, take note of partitions and recovery keys and whatnot
# burn iso with rufus (UEFI mode)
# disable RAID on BIOS, then enable it again

mkdir -p ~/Software/Dev
mkdir -p ~/.ssh
mkdir -p ~/.kube


sudo apt-get update 
sudo apt-get upgrade
sudo apt-get install \
curl \
cryfs \
exfat-fuse \
exfat-utils \
expect \
fabric \
gimp \
git \
git \
htop \
inkscape \
iotop \
jq \
keepassx \
mtr \
nmap \
openssh-client \
passwd \
pwgen \
python \
python-boto3 \
python-pip \
python3 \
python3-pip \
tcptraceroute \
terminator \
unzip \
vim \
vim-gtk \
wine-stable \
zip \
zsh \
zsh \
cheese

echo "OH MY ZSH"
sh -c "$(curl -fsSL https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"

echo "KUBECTL"
echo " as in https://kubernetes.io/docs/tasks/tools/install-kubectl/"
sudo apt-get update && sudo apt-get install -y apt-transport-https
curl -s https://packages.cloud.google.com/apt/doc/apt-key.gpg | sudo apt-key add -
sudo touch /etc/apt/sources.list.d/kubernetes.list
echo "deb http://apt.kubernetes.io/ kubernetes-xenial main" | sudo tee -a /etc/apt/sources.list.d/kubernetes.list
sudo apt-get update
sudo apt-get install -y kubectl

echo "MINIKUBE"
echo "https://github.com/kubernetes/minikube/releases"
echo "or directly curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.28.2/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/"

echo "FIREFOX"
echo "https://www.mozilla.org/de/firefox/developer/"

echo "VIRTUALBOX"
echo "https://www.virtualbox.org/wiki/Linux_Downloads"
echo "https://askubuntu.com/questions/760671/could-not-load-vboxdrv-after-upgrade-to-ubuntu-16-04-and-i-want-to-keep-secur/768310#768310"
echo "sudo apt-get install dkms build-essential linux-headers-`uname -r`"

echo "CONFIG:"
echo " - firefox"
echo " - keepassx"
echo " - manually install fantasque font from sysangel repo, files"

cd ~/Software/Dev
git clone https://github.com/angelalonso/sysangel

cd ~
mv .zshrc .zshrc_orig
ln -s ~/Software/Dev/sysangel/files/zshrc_home ~/.zshrc

mv ~/.config/terminator/config ~/.config/terminator/config_orig
ln -s ~/Software/Dev/sysangel/files/terminator_config ~/.config/terminator/config
