#!/usr/bin/env bash
#
## /etc/profile trigger for the automount of an encrypted volume


# Generating the keys:
# mkdir -p ~/.sysangel/keys
# openssl genrsa -out priv.key 4096
# openssl rsa -in ~/.sysangel/keys/priv.key -pubout > ~/.sysangel/keys/pub.key
# echo "${NEWPASS}" | openssl rsautl -inkey  ~/.sysangel/keys/pub.key -pubin -encrypt >  ~/.sysangel/keys/main_encfs.pass

USER=$(whoami)
HOME="/home/$USER"
FOLDRCONFIG="$HOME/.sysangel"
FOLDRKEYS="$FOLDRCONFIG/keys"
FLD_ENC_ORIG="$HOME/Dropbox/data/.enc"
FLD_ENC_DEST="$HOME/Priv"

TERM=$(which xterm)

### Functions

# Mount the encfs folder
mount_encfs(){
  PASS_ENCFS=$(/usr/bin/openssl rsautl -inkey $FOLDRKEYS/priv.key -decrypt < $FOLDRKEYS/cryfs.pass)

  CRYFS_FRONTEND=noninteractive echo "$PASS_ENCFS" | cryfs $FLD_ENC_ORIG $FLD_ENC_DEST

}

# Call the mount function
if [ "${USER}" = "aaf" ] ;then mount_encfs; fi
