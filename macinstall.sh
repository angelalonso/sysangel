#/bin/bash

echo "Installing Homebrew and Cask"
ruby -e "$(curl -fsSL "https://raw.githubusercontent.com/Homebrew/install/master/install")"
brew tap caskroom/cask
brew install caskroom/cask/brew-cask



