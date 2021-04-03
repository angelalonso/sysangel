#!/usr/bin/env bash
# Script to install the STaging version of Wine
# , lutris

set -e 
set -o pipefail


sudo dpkg --add-architecture i386
wget -nc https://dl.winehq.org/wine-builds/winehq.key
sudo apt-key add winehq.key
sudo apt-add-repository 'deb https://dl.winehq.org/wine-builds/ubuntu/ focal main'
sudo apt update
sudo apt install --install-recommends winehq-staging

sudo add-apt-repository ppa:lutris-team/lutris
sudo apt update
sudo apt install lutris

echo "check https://lutris.net/games/ubisoft-connect/ to install UPlay"

