#!/usr/bin/env bash
#
## /etc/profile trigger for the automount of Sysangel's config encrypted volume

USER=$(whoami)
FOLDRHOME="/home/$USER"
FOLDRCONFIG="$FOLDRHOME/.sysangel"
FOLDRDBOX="${FOLDRHOME}/Dropbox"
FOLDRKEYS="${FOLDRCONFIG}/KEYS"
FLD_ENC_ORIG="${FOLDRDBOX}/.${USER}.encfs"
FLD_ENC_DEST="${FOLDRCONFIG}/${USER}.private"


TERM=$(which xterm)

PASS_ENCFS=$(/usr/bin/openssl rsautl -inkey $FOLDRKEYS/priv.key -decrypt < $FOLDRKEYS/encfs.pass)

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

