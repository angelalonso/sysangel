#!/usr/bin/env bash
#
## /etc/profile trigger for the sysadmin python script

PYTHON=$(which python)
TERM=$(which xterm)

${TERM} -e "${PYTHON} ${HOME}/sysangel/sysangel.py"
