#!/usr/bin/env bash

#If a command fails, make the whole script exit
#Treat unset variables as an error, and immediately exit.
#Disable filename expansion (globbing) upon seeing *, ?, etc..
#Produce a failure return code if any command errors
set -euf -o pipefail
 
DEBIAN_FRONTEND=noninteractive
PKGS="\
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
zsh \
"
#-# Update system
sudo apt-get update && sudo DEBIAN_FRONTEND=noninteractive apt-get upgrade -yq

#-# Install packages (apt, pip, other manual)
# chrome_deb, firefox developer sync
# rambox
# atom
# dropbox

