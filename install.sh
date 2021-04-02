#!/bin/bash

mkdir -p ~/Software/Dev
mkdir -p ~/Software/Work
mkdir -p ~/.ssh
mkdir -p ~/.kube


sudo apt-get update 
sudo apt-get upgrade
sudo apt-get install \
apt-transport-https \
arp-scan \
chromium \
cryfs \
curl \
gimp \
git \
htop \
inkscape \
iotop \
jq \
keepassx \
mtr \
net-tools \
nmap \
pwgen \
terminator \
unzip \
vim \
vlc \
zip \
zsh

curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
unzip awscliv2.zip
sudo ./aws/install


## TO BE PREPARED:
# - Before reinstalling
# firefox dev edition
# docker
# dropbox
# vimrc
# terminator config
# - After reinstalling
# kubectl
# virtualbox
# terraform
# saml2aws

