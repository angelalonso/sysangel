#!/usr/bin/env bash

#If a command fails, make the whole script exit
#Treat unset variables as an error, and immediately exit.
#Disable filename expansion (globbing) upon seeing *, ?, etc..
#Produce a failure return code if any command errors
set -euf -o pipefail


#-# Update system
#-# Install packages (apt, pip, other manual)
#-# Configure (programs, keys)
