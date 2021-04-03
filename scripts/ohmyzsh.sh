#!/usr/bin/env bash
# Installs OhMyZsh
if [ ! -d "$HOME/.oh-my-zsh" ]; then
  sh -c "$(wget https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh -O -)"
  exit
else
  echo "Oh My ZSH already installed"
fi
