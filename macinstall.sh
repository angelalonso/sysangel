#/bin/bash

# http://lifehacker.com/how-to-make-your-own-bulk-app-installer-for-os-x-1586252163
echo "Installing Homebrew and Cask"
ruby -e "$(curl -fsSL "https://raw.githubusercontent.com/Homebrew/install/master/install")"
sudo chown -R $(whoami) /usr/local
brew tap caskroom/cask
brew install caskroom/cask/brew-cask



brew cask install git macports homebrew/fuse/encfs firefox google-chrome vim libreoffice keepassx dropbox franz virtualbox
echo
echo "TASK: MOVE FIREFOX, CHROME, LIBREOFFICE TO DOCK"
echo
echo "TASK: CREATE KEYS FOR FIREFOX, CHROME, KEEPASSX"
echo
echo "TASK: LOGIN WITH USER INTO FIREFOX, CHROME, DROPBOX, FRANZ"
echo
echo "TASK: COPY OVER SSH KEYS, GPG KEYS"

# automount Private

# Install Commandlinetools package (clang make...)
xcode-select --install

# reinstall python manually
brew install python3 python


# vim config

# oh my zsh
sh -c "$(curl -fsSL https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"

# CONFIG CHANGES:
# System Preferences > Keyboard > Shortcuts > App Shortcuts
#  iTerm, Split Vertically with Current Profile CTRL+SHIFT+d

