#!/usr/bin/env bash

# GLOBAL VARS
SSHPATH="$HOME/.ssh"
SSHFRIDGEPATH="$SSHPATH/.fridge"
GREENLIST="$SSHPATH/.list"


function turn_on {
  echo ON $1
  if [ "$1" == "" ]; then
    echo " Which key do you want to enable?:"
    echo 
    LIST=$(grep KEY $SSHFRIDGEPATH/* 2>/dev/null | awk -F":" '{print $1}' | sort | uniq)
    for i in $LIST; do
      echo "$i"
    done
    echo 
    read -e -p "Paste the path here:" SSHFILEPATH
    mv $SSHFILEPATH $SSHPATH/
    echo "moved $SSHFILEPATH out of the Fridge"
  fi

}

function turn_off {
  mkdir -p $SSHFRIDGEPATH
  LIST=$(grep KEY $SSHPATH/* 2>/dev/null | awk -F":" '{print $1}' | sort | uniq)
  for i in $LIST; do
    GREENED=False
    for j in $(cat $GREENLIST); do
      if [[ "$i" == *"$j"* ]]; then
        echo "Not moving $i to the fridge!"
        GREENED=True
      fi
    done
    if [ "$GREENED" == "False" ]; then
      echo "moving $i to the Fridge"
      mv $i $SSHFRIDGEPATH/
    fi
  done

}

function help {
  echo SYNTAX:
  echo " $0 [on|off]"
}

case $1 in
  "on")
    turn_on $2
    ;;
  "off")
    turn_off
    ;;
  *)
    help
    ;;
esac
