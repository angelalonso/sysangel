#!/usr/bin/env bash

BASEPATH="$HOME/Software/Work"

CWD=$(pwd)
for i in $(ls $BASEPATH | grep $1); do
  printf "$i \t- "
  cd $BASEPATH/$i && git branch --show-current
  cd $CWD
done
