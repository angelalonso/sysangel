#!/usr/bin/env bash
#
## /etc/profile trigger for the automount of an encrypted volume


# Generating the keys:
# mkdir -p ~/.sysangel/KEYS
# openssl genrsa -out priv.key 4096
# openssl rsa -in ~/.sysangel/KEYS/priv.key -pubout > ~/.sysangel/KEYS/pub.key
# echo "${NEWPASS}" | openssl rsautl -inkey  ~/.sysangel/KEYS/pub.key -pubin -encrypt >  ~/.sysangel/KEYS/main_encfs.pass

USER=$(whoami)
HOME="/home/$USER"
FOLDRCONFIG="$HOME/.sysangel"
FOLDRKEYS="$FOLDRCONFIG/KEYS"
FLD_ENC_ORIG="$HOME/Dropbox/data/.encrypted"
FLD_ENC_DEST="$HOME/Private"

TERM=$(which xterm)

### Functions

# Mount the encfs folder
mount_encfs(){
  PASS_ENCFS=$(/usr/bin/openssl rsautl -inkey $FOLDRKEYS/priv.key -decrypt < $FOLDRKEYS/main_encfs.pass)

  echo $PASS_ENCFS | encfs -S $FLD_ENC_ORIG $FLD_ENC_DEST

}

# Call the mount function
if [ "${USER}" = "aaf" ] ;then
  mount_encfs
fi

# Wait for the user to know what happened
#   (this is meant to run in a terminal that will close afterwards)
#echo "Press a key to continue"
#read -n 1
