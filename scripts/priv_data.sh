#!/bin/bash

PRIVDIR=${HOME}/Private/config_secret
BACKDIR=${HOME}/Private_offline/config_secret
AWSDIR=${HOME}/.aws
SSHDIR=${HOME}/.ssh

if [ "$(ls -A "$PRIVDIR" 2> /dev/null)" == "" ]; then
    rm -rf ${AWSDIR} 2>/dev/null
    ln -s ${BACKDIR}/.aws ${AWSDIR}
    rm -rf ${SSHDIR} 2>/dev/null
    ln -s ${BACKDIR}/.ssh ${SSHDIR}
  else
    rm -rf ${AWSDIR} 2>/dev/null
    ln -s ${PRIVDIR}/.aws ${AWSDIR}
    rm -rf ${SSHDIR} 2>/dev/null
    ln -s ${PRIVDIR}/.ssh ${SSHDIR}
fi
