# Sysangel

Script to get my laptops up and running with as little manual effort as possible

# Installation

## Ubuntu - tested
apt-get install git
cd ~ && git clone https://github.com/angelalonso/sysangel && cd sysangel
bash install.sh

# Status

Being rewritten into separated scripts, to make it more sustainable

# References for myself

What I need now is a script that simply gets me up and running with all things I use in a regular basis:
- For the main use this will suffice:
  - Minimum list of packages and applications I normally use
  - SSH agent autoloading
  - aws, kube, ssh folders
  - Dropbox and privd configured
  - Chrome and Firefox, which I then sync afterwards
  - my regular config files (zshrc, vimrc)

References for more specific configurations:
- Mouse touchpad click
  - Only for the eeepc
  http://askubuntu.com/questions/690138/synaptics-touchpad-tap-to-click-is-not-working-in-ubuntu-15-10gnome
  synclient TapButton1=1 TapButton2=3 TapButton3=2
- Key Shortcuts
  - iTerm profile (#<-, #->) save here
  - Map iTerm keybindings
  - Map # as Win?

- On a future phase, it should be capable of cleaning up what it did

