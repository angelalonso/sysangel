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
FLD_BACKUP="$HOME/Priv_offline"
GITDIR="${HOME}/sysangel"

### Functions

sync_priv(){
  if [ "$(ls -A "$FLD_ENC_DEST" 2> /dev/null)" != "" ]; then
    rsync -rtvu ${FLD_ENC_DEST}/ ${FLD_BACKUP}/ 2>/dev/null
  fi
}

# Mount the encfs folder
mount_encfs(){
  PASS_ENCFS=$(/usr/bin/openssl rsautl -inkey $FOLDRKEYS/priv.key -decrypt < $FOLDRKEYS/cryfs.pass)

  CRYFS_FRONTEND=noninteractive echo "$PASS_ENCFS" | cryfs $FLD_ENC_ORIG $FLD_ENC_DEST

  sync_priv
  ${GITDIR}/scripts/priv_data.sh

}

# Call the mount function
if [ "${USER}" = "aaf" ] ;then mount_encfs; fi
