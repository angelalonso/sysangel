#!/bin/bash


# Check mounted Priv does not delay use of ssh
/home/aaf/sysangel/scripts/priv_data.sh

# Compose keys -> RCTRL + (n ~) = ñ
setxkbmap  -option compose:rctrl
xmodmap ~/.Xmodmap


xmodmap -e 'clear Lock' -e 'keycode 0x42 = Escape'


## Detect and configure touchpad. See 'man synclient' for more info.
if egrep -iq 'touchpad' /proc/bus/input/devices; then
    synclient VertEdgeScroll=1 TapButton1=1 &
fi
