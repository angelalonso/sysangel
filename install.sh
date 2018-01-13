#!/usr/bin/env bash

#If a command fails, make the whole script exit
#Treat unset variables as an error, and immediately exit.
#Disable filename expansion (globbing) upon seeing *, ?, etc..
#Produce a failure return code if any command errors
set -euf -o pipefail

FLDR_MAIN=$(dirname $0)
FLDR_SCRIPTS="$FLDR_MAIN/scripts"


bash $FLDR_SCRIPTS/install_packages.sh
echo "Packages installed"

#-# Update system
# apt-get update && apt-get upgrade

#-# Install packages (apt, pip, other manual)
# chrome_deb, firefox developer sync
# rambox
# atom
# dropbox

#-# Configure (programs, keys)
# ohmyzsh
# vim
# firefox developer sync
# thunderbird copy from HDD
# terminator
# fonts
# caps2esc
# awscli
# kubectl
# terraform
# docker
# helm
# kops
# privd
