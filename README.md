# Sysangel v.whatever

Script to get my laptops up and running with as little manual effort as possible
NOTE: previous failures can be found under OLD/

## Background

What I need now is a script that simply gets me up and running with all things I use in a regular basis:
- Dropbox
- Automount of encrypted folder
- SSH agent autoloading
- Chrome and Firefox, which I then sync afterwards
...

## How it should work

install.sh
- Installs all dependencies for sysangel.py to work
- Calls the actual sysangel.py script with "install" as parameter

sysangel.py
- on install mode:
  - tbd
- on update mode:
  - tbd

### Prerequisites

git, install it however you like


## Installation

cd ~ && git clone https://github.com/angelalonso/sysangel && cd sysangel && bash install.sh

IMPORTANT: it MUST be installed on your HOME directory to work properly
