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
  $CWD/alarm.sh
}

fn[ip]=run_getip
hlp[ip]="get the public IP for this machine"
function run_getip {
  curl curlmyip.net
}

fn[repo]=run_multirepo
hlp[repo]="show the branches on a list of repos from a given work directory"
function run_multirepo {
  $CWD/multirepo.sh
}

fn[ssh]=run_ssh
hlp[ssh]="to be determined"
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


if [[ $# == 0 ]]; then
  menu
else
  check_fn $1
fi


