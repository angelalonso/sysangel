#!/bin/bash

PRIVDIR=${HOME}/Priv
BACKDIR=${HOME}/Priv_offline
AWSDIR=${HOME}/.aws
SSHDIR=${HOME}/.ssh
KUBEDIR=${HOME}/.kube
CREDSDIR=${HOME}/.credentials

if [ "$(ls -A "$PRIVDIR" 2> /dev/null)" == "" ]; then
    rm -rf ${SSHDIR} 2>/dev/null
    ln -s ${BACKDIR}/.ssh ${SSHDIR}
    rm -rf ${KUBEDIR} 2>/dev/null
    ln -s ${BACKDIR}/.kube ${KUBEDIR}
    rm -rf ${CREDSDIR} 2>/dev/null
    ln -s ${BACKDIR}/.kube ${CREDSDIR}
    # kubectl and docker and terraform do not like .aws to be a link
    mkdir -p ${AWSDIR}
    rsync -avzh ${BACKDIR}/.aws/ ${AWSDIR}

  else
    rm -rf ${SSHDIR} 2>/dev/null
    ln -s ${PRIVDIR}/.ssh ${SSHDIR}
    rm -rf ${KUBEDIR} 2>/dev/null
    ln -s ${PRIVDIR}/.kube ${KUBEDIR}
    rm -rf ${CREDSDIR} 2>/dev/null
    ln -s ${PRIVDIR}/.credentials ${CREDSDIR}
    # kubectl and docker and terraform do not like .aws to be a link
    mkdir -p ${AWSDIR}
    rsync -avzh ${PRIVDIR}/.aws/ ${AWSDIR}
fi
