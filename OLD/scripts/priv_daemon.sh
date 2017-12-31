#!/usr/bin/env bash
#
# Tool to handle mounting and maintenance of the private folders
# TODO:
# mount both -> on a second phase, mount only the main one
# check everything is fine before syncing (keepass? if no lock file, then sync)
# sync every x (5?) minutes
# check if anything goes wrong,  switch back to saved version


# Generating the keys:
# mkdir -p ~/.sysangel/keys
# openssl genrsa -out priv.key 4096
# openssl rsa -in ~/.sysangel/keys/priv.key -pubout > ~/.sysangel/keys/pub.key
# echo "${NEWPASS}" | openssl rsautl -inkey  ~/.sysangel/keys/pub.key -pubin -encrypt >  ~/.sysangel/keys/cryfs.pass

USER=$(whoami)
HOME="/Users/$USER"
FOLDRCONFIG="$HOME/.sysangel"
FOLDRKEYS="$FOLDRCONFIG/keys"
FLD_ORIG="$HOME/Dropbox/.enc_a"
FLD_ORIG_B="$HOME/Dropbox/.enc_b"
FLD_DEST="$HOME/Private"
FLD_DEST_B="$HOME/Private.bck"

TERM=$(which xterm)

### Functions

# Mount the encfs folder
mount_cryfs(){
  PASS_CRYFS=$(/usr/bin/openssl rsautl -inkey $FOLDRKEYS/priv.key -decrypt < $FOLDRKEYS/cryfs.pass)
  
  CRYFS_A=noninteractive echo "$PASS_CRYFS" | cryfs $FLD_ORIG $FLD_DEST
  CRYFS_B=noninteractive echo "$PASS_CRYFS" | cryfs $FLD_ORIG_B $FLD_DEST_B
}

mount_cryfs
