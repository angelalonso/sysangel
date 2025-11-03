#!/bin/bash

# Dirs
WORK="$HOME"
LOCK="$WORK/backup_wrapped.$1.lock"
USBDIR="/media/$USER/$1"

# Execs
#BLKID=$(which blkid)
GREP=$(which grep)
LS=$(which ls)
WC=$(which wc)

backup(){
  DIR=$1
  rsync -avzh $WORK/$DIR/ $USBDIR/$DIR
}


# Variables
MAX_TIME=5000

# Check that no other instance is running
if [ -e $LOCK ]; then
  echo "There is a Lock file at $LOCK." 
  echo "Please check that another backup is not running, then delete that and try again!"
  read -n 1 -s
  exit 2
else
  touch $LOCK

  # Loop that waits for the volume to be mounted
  USB_IN=0
  timeout=0
  while [ $USB_IN -eq 0 ] && [ $timeout -lt $MAX_TIME ]
  do
    USB_IN=$($BLKID | $GREP $1 | $WC -l)
    USB_IN=$($LS $USBDIR 2>/dev/null | $WC -l)
    timeout=`expr $timeout + 1`
  done
  
  # When it finally is mounted, we check again and then proceed
  if [ $timeout -lt $MAX_TIME ]; then
#    sleep 1
#    echo "$1 Mounted!"
    # Here we read the list to be backed up from a file in the USB itself
    IFS=$'\r\n' GLOBIGNORE='*' :; BACKUPDIRS=($(cat $USBDIR/backup.list | grep -v '^#'))
    echo "Today we'll be backing up the following dirs into $USBDIR:"
    echo "${BACKUPDIRS[@]}"
    echo "Press any key to continue, CTRL+C (Alt+F4) to exit"
    read -n 1 -s

    for i in "${BACKUPDIRS[@]}"
    do
      # Then back the hell out of it!
      echo "Backing $i up..."
      backup $i
    done
    echo "Press any key to exit..."
    read -n 1 -s

  # If it took too long for the volume to mount, we are done.
  else
    echo "The volume was not mounted on time, or mounted somewhere else."
    echo "Expecting "$USBDIR
    $LS -l /media/$USER
    echo 
    echo "Press any key to exit..."
    read -n 1 -s
    rm $LOCK
    exit 2
  fi
fi
rm $LOCK
