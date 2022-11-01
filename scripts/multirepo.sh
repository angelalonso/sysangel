#!/usr/bin/env bash

BASEPATH="$HOME/Software/Work"

if [[ $1 == "" ]]; then
  read -p "Enter a string to grep for repos on $BASEPATH: " REPOGREP
else
  REPOGREP=$1
fi


CWD=$(pwd)
for i in $(ls $BASEPATH | grep $REPOGREP); do
  printf "$BASEPATH/$i \t- "
  cd $BASEPATH/$i && git branch --show-current
  cd $CWD
done
