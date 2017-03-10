#/bin/bash

# http://lifehacker.com/how-to-make-your-own-bulk-app-installer-for-os-x-1586252163
echo "Installing Homebrew and Cask"
ruby -e "$(curl -fsSL "https://raw.githubusercontent.com/Homebrew/install/master/install")"
sudo chown -R $(whoami) /usr/local/Cellar
sudo chown -R $(whoami) /usr/local/Homebrew
sudo chown -R $(whoami) /usr/local/var/homebrew
sudo chown -R $(whoami) /usr/local/zsh
brew tap caskroom/cask
brew install caskroom/cask/brew-cask
sudo chown -R $(whoami) /usr/local/Caskroom

# oh my zsh
# vim

brew cask install firefox
brew cask install chrome libreoffice keepass
