#!/bin/bash

# Dirs
# USAGE: $HOME/backup.sh EXTERNAL_SSD_MOUNT_FOLDER

WORK="$PWD"
LOCK="$WORK/backup.$1.lock"


# Execs
SCRIPT="$WORK/backup_wrapped.sh"

TERM=$(which xterm)

if [ -e $LOCK ]; then
  $TERM -e "Please, clean up the lock file"
  read ans
  exit 1
else
  touch $LOCK
  $TERM -e $SCRIPT $1 &
fi
rm $LOCK
