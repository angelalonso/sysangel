#!/bin/sh
mkdir -p ~/Pictures
FILE="$HOME/Pictures/Screenshot_$(date +'%Y-%m-%d_%H-%M-%S').png"

# Capture region selected with slurp
if grim -g "$(slurp)" "$FILE"; then
    wl-copy < "$FILE"
fi
