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

brew cask install git firefox google-chrome libreoffice keepassx dropbox virtualbox
echo
echo "TASK: MOVE FIREFOX, CHROME, LIBREOFFICE TO DOCK"
echo
echo "TASK: CREATE KEYS FOR FIREFOX, CHROME, KEEPASSX"
echo
echo "TASK: LOGIN WITH USER INTO FIREFOX, CHROME"

# automount Private

# Install Commandlinetools package (clang make...)
xcode-select --install

# reinstall python manually
brew install python3 python


# vim

# oh my zsh
sh -c "$(curl -fsSL https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"
