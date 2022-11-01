#!/bin/bash
clear

if [[ $1 == "" ]]; then
  read -p "Enter the time for the Alarm (E.g.: $(date '+%H:%M')) " ALARM
else
  ALARM=$1
fi

sleep $(( $(date --date="$ALARM" +%s) - $(date +%s) ));

echo -e "\e[41m\e[5mALARM! (Press CTRL+c to stop)"
notify-send "ALARM at $ALARM"
while true;
do
  spd-say 'MAY I HAVE YOUR ATTENTION, PLEASE?'
  sleep 2; 
done

