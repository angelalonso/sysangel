#!/usr/bin/env bash

# We define functions and store them on a dictionary
declare -A fn

fn[alarm]=run_alarm
function run_alarm {
  ./alarm.sh
}

fn[repo]=run_multirepo
function run_multirepo {
  ./multirepo.sh
}

fn[ssh]=run_ssh
function run_ssh {
  echo "ssh script TBD"
}

function check_fn {
  ${fn[$1]}
}

function menu {
  chosen=$(for i in "${!fn[@]}"; do echo $i; done | fzf)
  ${fn[$chosen]}
}

function help {
 echo "SYNTAX: $0 <action>"
 echo ""
 echo "- Actions available:"
 echo " - alarm"
 echo "${!fn[@]}"
}

if [[ $1 == "" ]]; then
  menu
else
  check_fn $1
fi


