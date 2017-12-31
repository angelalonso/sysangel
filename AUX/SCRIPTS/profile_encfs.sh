#!/usr/bin/env bash
#
## /etc/profile trigger for the automount of an encrypted volume

USER=$(whoami)
HOME="/home/$USER"
FOLDRCONFIG="$HOME/.sysangel"
FOLDRKEYS="$FOLDRCONFIG/KEYS"
FLD_ENC_ORIG="$HOME/Dropbox/data/.encrypted"
FLD_ENC_DEST="$HOME/Private"

TERM=$(which xterm)

PASS_ENCFS=$(/usr/bin/openssl rsautl -inkey $FOLDRKEYS/private.key -decrypt < $FOLDRKEYS/encfs.pass)

### Functions

# Mount the encfs folder
mount_encfs(){
  expect <<- DONE
    spawn encfs $FLD_ENC_ORIG $FLD_ENC_DEST
    expect "EncFS Password:"
    send    "$PASS_ENCFS\n"
    expect eof
DONE

}

# Call the mount function
mount_encfs

# Wait for the user to know what happened
#   (this is meant to run in a terminal that will close afterwards)
#echo "Press a key to continue"
#read -n 1
