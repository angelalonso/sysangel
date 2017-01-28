# Sysangel v.whatever

Script to get my laptops up and running with as little manual effort as possible
NOTE: previous failures can be found under OLD/
NOTE2: as much as I'd like to improve this, I'm starting with support only for XFCE (yeah, I'm that mediocre!)

## Background

What I need now is a script that simply gets me up and running with all things I use in a regular basis:
- Dropbox - DONE
- Automount of encrypted folder
  - TO TEST: key generation, autorun of profile script
- TO DO: SSH agent autoloading
- Chrome and Firefox, which I then sync afterwards
...

## How it should work

install.sh
- Installs all dependencies for sysangel.py to work
- Calls the actual sysangel.py script with "install" as parameter

sysangel.py
- on install mode:
  - Installs all required packages - WIP
  - Installs Dropbox - DONE
    - Would need error handling
  - Sets up shared encrypted volume - TBD
- on update mode:
  - Installs required packages - TBD
  - Accepts parameters to only update one part - TBD

### Prerequisites

git, install it however you like


## Installation

cd ~ && git clone https://github.com/angelalonso/sysangel && cd sysangel && bash install.sh

IMPORTANT: it MUST be installed on your HOME directory to work properly


# Personal Goals

- One script that sets up everything after a fresh install. Apart from the above, I'd also want...
  - Vim 8.0, vimrc and plugins
  - Mouse touchpad click
  - Oh My Zsh
  - Key Shortcuts
  - Firefox Sync
