# Sysangel

Script to get my laptops up and running with as little manual effort as possible

# Status

Being rewritten into separated scripts, to make it more sustainable

# References for myself

What I need now is a script that simply gets me up and running with all things I use in a regular basis:
- For the main use this will suffice:
  - Dropbox
  - Automount of encrypted folder
    - Key generation, autorun of profile script
  - SSH agent autoloading
  - Chrome and Firefox, which I then sync afterwards
- For long term removal of annoyances:
  - Any minimal package that I found I needed at the msot inconvenient time
  - Unified configuration of tools, mainly keybindings (probably mirroring Mac's ones to XFCE)
...
- One script that sets up everything after a fresh install.
- On a first phase, it installs all straightforward packages needed.
  - Chrome and Firefox, which I then sync afterwards
  - ...
- On a second phase, it installs the not-so.easy-to-install ones (one script or function per program)
  - Dropbox - DONE
- On a third phase, it copies configuration files and scripts until the behaviour is the same across machines
  - Automount of encrypted folder
    - key generation, autorun of profile script
  - SSH agent autoloading
  - Mouse touchpad scroll, mac-like
    Deprecated because in any case, Chromium will ignore.
    Since I don't have a preference, I'd change the Mac to Regular scrolling instead
    synclient VertScrollDelta=-77 HorizScrollDelta=-77
  - Mouse touchpad click
    http://askubuntu.com/questions/690138/synaptics-touchpad-tap-to-click-is-not-working-in-ubuntu-15-10gnome
    synclient TapButton1=1 TapButton2=3 TapButton3=2
  - Key Shortcuts
    - iTerm profile (#<-, #->) save here
    - Map iTerm keybindings
    - Map # as Win?
  - Firefox Sync

- On a future phase, it also is capable of cleaning up what it did

