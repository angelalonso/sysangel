#!/usr/bin/env bash

# GLOBAL VARS
SSHPATH="$HOME/.ssh"
SSHFRIDGEPATH="$SSHPATH/.fridge"
GREENLIST="$SSHPATH/.list"


function enable-keys {
  echo ON $1
  if [ "$1" == "" ]; then
    echo " Which key do you want to enable?:"
    echo 
    LIST=$(grep -l KEY $SSHFRIDGEPATH/* 2>/dev/null | sort | uniq)
    for i in $LIST; do
      echo "$i"
    done
    echo 
    read -e -p "Paste the path here:" SSHFILEPATH
    mv $SSHFILEPATH $SSHPATH/
    echo "moved $SSHFILEPATH out of the Fridge"
  fi
  NEWSSHFILEPATH=$(echo $SSHFILEPATH | sed 's#\.fridge/##g')
  ssh-add $NEWSSHFILEPATH
  refresh-agent
}

function disable-all-keys {
  mkdir -p $SSHFRIDGEPATH
  #LIST=$(grep KEY $SSHPATH/* 2>/dev/null | awk -F":" '{print $1}' | sort | uniq)
  LIST=$(grep -l KEY $SSHPATH/* 2>/dev/null | sort | uniq)
  for i in $LIST; do
    GREENED=False
    for j in $(cat $GREENLIST); do
      if [[ "$i" == "$SSHPATH/$j"* ]]; then
        echo "Not moving $i to the fridge!"
        GREENED=True
      fi
    done
    if [ "$GREENED" == "False" ]; then
      echo "moving $i to the Fridge"
      mv $i $SSHFRIDGEPATH/
    fi
  done
  refresh-agent
}

function refresh-agent {
  echo
  echo "Loaded SSH Keys:"
  ssh-add -l
  echo
  while true; do
    read -p "Do you want to refresh the SSH Agent loaded keys? " yn
    case $yn in
      [Yy]* ) ssh-add -D;
        LIST=$(grep KEY $SSHPATH/* 2>/dev/null | awk -F":" '{print $1}' | sort | uniq)
        for i in $LIST; do
          ssh-add $i
        done
        echo 
        echo "Loaded SSH Keys:"
        ssh-add -l
        break;;
      [Nn]* ) 
        exit;;
      * ) echo "Please answer yes or no.";;
    esac
  done
}

function modify-greenlist {
  vim $GREENLIST
}

function help {
  echo SYNTAX:
  echo " $0 [out|in|greenlist]"
  echo "   $0 out        - takes a key out of the fridge, ready to use"
  echo "   $0 in         - puts all key on .ssh into the fridge, disabling them"
  echo "   $0 greenlist  - Edits the Greenlist of keys that must not go into the fridge"
}

case $1 in
  "out")
    enable-keys $2
    ;;
  "in")
    disable-all-keys
    ;;
  "green"|"greenlist")
    modify-greenlist
    ;;
  *)
    help
    ;;
esac
