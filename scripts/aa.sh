#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail
if [[ "${TRACE-0}" == "1" ]]; then
  set -o xtrace
fi

# 'a' and the script live in the same folder but can be called from anywhere
CWD=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# We define functions and store them on a dictionary
declare -A fn
declare -A hlp

fn[alarm]=run_alarm
hlp[alarm]="add an alarm at a given time of the day (HH:MM)"
function run_alarm {
  shift 1
  $CWD/alarm.sh $@ 
}

fn[ip]=run_getip
hlp[ip]="get the public IP for this machine"
function run_getip {
  curl curlmyip.net
}

fn[moon]=run_moon
hlp[moon]="show moon phase"
function run_moon {
  curl https://wttr.in/Moon
}

fn[repo]=run_multirepo
hlp[repo]="show the branches on a list of repos from a given work directory"
function run_multirepo {
  shift 1
  $CWD/multirepo.sh "$@"
}

fn[test]=run_test
hlp[ssh]="to be determined"
function run_test {
  shift 1
  echo parameters: "$@"
}

fn[weather]=run_weather
hlp[weather]="show weather for Berlin"
function run_weather {
  curl https://wttr.in/Berlin,Germany
}

function check_fn {
  ${fn[$1]} "$@"
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


if [[ $# == 0 ]]; then
  menu
else
  check_fn "$@"
fi


