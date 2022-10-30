#!/bin/bash
clear

if [[ $1 == "" ]]; then
  echo "ERROR: Not a valid Alarm received!"
  echo "  SYNTAX:"
  echo "$0 <Alarm Time in HH:MM Format>"
  echo "  E.g.: $0 $(date '+%H:%M')"
  exit 2
else
  ALARM=$1

  sleep $(( $(date --date="$ALARM" +%s) - $(date +%s) ));

  echo -e "\e[41m\e[5mALARM!"
  notify-send "ALARM at $ALARM"
fi
