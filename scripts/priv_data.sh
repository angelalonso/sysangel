#!/bin/bash

PRIVDIR=${HOME}/Private/config_secret
BACKDIR=${HOME}/Private_offline/config_secret
AWSDIR=${HOME}/.aws
SSHDIR=${HOME}/.ssh
KUBEDIR=${HOME}/.kube

if [ "$(ls -A "$PRIVDIR" 2> /dev/null)" == "" ]; then
    rm -rf ${SSHDIR} 2>/dev/null
    ln -s ${BACKDIR}/.ssh ${SSHDIR}
    rm -rf ${KUBEDIR} 2>/dev/null
    ln -s ${BACKDIR}/.kube ${KUBEDIR}
    mkdir -p ${AWSDIR}
    rsync -avzh ${BACKDIR}/.aws/ ${AWSDIR}

  else
    rm -rf ${SSHDIR} 2>/dev/null
    ln -s ${PRIVDIR}/.ssh ${SSHDIR}
    rm -rf ${KUBEDIR} 2>/dev/null
    ln -s ${PRIVDIR}/.kube ${KUBEDIR}
    mkdir -p ${AWSDIR}
    rsync -avzh ${PRIVDIR}/.aws/ ${AWSDIR}
fi
