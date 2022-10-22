#!/usr/bin/bash

SAVEIFS=$IFS
IFS=$(echo -en "\n\b")

for i in $(ls $1/*.mp4)
do
  MP3FILENAME=$(echo $i | sed 's/.mp4/.mp3/g')
  ls "$MP3FILENAME" >/dev/null 2>&1
  FOUND=$?
  if [[ $FOUND -ne 0 ]]; then
    echo -n ""
#    echo "- Keeping $i, $MP3FILENAME not found"
  else
    if [ -s $MP3FILENAME ]; then
      echo "- REMOVING $i, $MP3FILENAME exists"
#      rm $i
#      echo -n ""
#    else
#      echo "- $MP3FILENAME size is ZERO"
    fi
  fi
done

IFS=$SAVEIFS
echo
echo "THIS WAS A DRY RUN, PLEASE MODIFY SCRIPT $0 TO REMOVE THE FILES"
