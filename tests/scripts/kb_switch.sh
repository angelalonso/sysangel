#!/usr/bin/env bash

switch_to_int () {
  for id in $(xinput -list | grep 'AT Translated Set 2 keyboard' | cut -f 2 | cut -d= -f 2); do
    echo Setting $id MAIN
    # NOTE: I swap Left Alt and Left Win to make the layout mroe similar to Mac's
    setxkbmap -device $id -layout de -variant nodeadkeys -option "altwin:swap_lalt_lwin" 
    #setxkbmap -device $id -layout de -variant nodeadkeys -option "ctrl:swap_lwin_lctl" -option "altwin:swap_lalt_lwin" FAILS
    #setxkbmap -device $id -layout de -variant nodeadkeys -option "ctrl:swap_lalt_lctl_lwin"
    #setxkbmap -device $id -layout de -option altwin:swap_lalt_lwin -option lv3:lalt_switch
    #setxkbmap -device $id -layout de -option altwin:swap_alt_win
  done
}

switch_to_ext () {
  for id in $(xinput -list | grep 'Angel Alonso' | cut -f 2 | cut -d= -f 2); do
    echo Setting $id EXT
    setxkbmap -device $id -layout us -variant mac
  done
  echo 
  echo "DONE! Press <Enter> to close this window..."
  read ans
}

# NOTE: All this is because I cannot have two separate sets and work simultaneously (Eg: I switch Alt and Win for internal, and external has it swapped too)
EXT=$(xinput -list | grep 'Angel Alonso' | cut -f 2 | cut -d= -f 2)
if [[ "$EXT" == "" ]]; then
  echo "no EXT connected"
  switch_to_int
else
  echo "EXT connected is connected, do you want to switch to US Mac Keyboard Layout?"
  while true; do
      read -p "$* [y/n]: " yn
      case $yn in
          [Yy]*) switch_to_ext; exit 0 ;;  
          [Nn]*) switch_to_int; exit 0 ;;
      esac
  done
fi
