#!/usr/bin/env bash
# Script to install the latest version of firefox dev edition

set -e 
set -o pipefail

mkdir -p ~/.local/bin
if [ ! -f "firefox.tar.bz2" ]; then
  echo "   ...downloading..."
  wget "https://download.mozilla.org/?product=firefox-devedition-latest-ssl&os=linux64&lang=en-US" -O firefox.tar.bz2
fi
if [ $(ls ~/.local/bin/firefox/* | wc -l) -lt 1 ]; then
  echo "   ...uncompressing..."
  tar -xvf firefox.tar.bz2 -C ~/.local/bin
fi
if [ -f "/usr/bin/firefox" ]; then
  sudo rm /usr/bin/firefox
fi
sudo ln -s ~/.local/bin/firefox/firefox /usr/bin
