#!/bin/bash


# Check mounted Priv does not delay use of ssh
/home/aaf/sysangel/scripts/priv_data.sh

# Compose keys -> RCTRL + (n ~) = ñ
setxkbmap  -option compose:rctrl
# map that switches ctrl and SuperL (mac style)
xmodmap ~/.Xmodmap
# Caps Lock used as another Esc
xmodmap -e 'clear Lock' -e 'keycode 0x42 = Escape'
# This makes CTRL (the key) usable in the terminal
#   Only usable with the previous mac style xmodmap
/usr/bin/xbindkeys -f /home/aaf/.xbindkeysrc

## Detect and configure touchpad. See 'man synclient' for more info.
if egrep -iq 'touchpad' /proc/bus/input/devices; then
    synclient VertEdgeScroll=1 TapButton1=1 TapButton2=3 TapButton3=2 &
fi
