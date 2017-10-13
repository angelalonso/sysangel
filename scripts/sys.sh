#!/usr/bin/env bash
#DOCKER="docker"
DOCKER="sudo docker"
NAME="sysadmin"
IMG="angelalonso/sysadmin:v0.03"

TESTDIR="$HOME/.ssh"
if [[ -L $TESTDIR ]]; then
  SSHDIR=$(readlink $TESTDIR)
else
  SSHDIR=$TESTDIR
fi

TESTDIR="$HOME/.aws"
if [[ -L $TESTDIR ]]; then
  AWSDIR=$(readlink $TESTDIR)
else
  AWSDIR=$TESTDIR
fi

TESTDIR="$HOME/.kube"
if [[ -L $TESTDIR ]]; then
  KUBEDIR=$(readlink $TESTDIR)
else
  KUBEDIR=$TESTDIR
fi
run_n_enter() {
#  $DOCKER run -it -v ${HOME}/.aws:/root/.aws -v ${SSHDIR}:/root/.ssh -v ${HOME}/.kube:/root/.kube --name $NAME $IMG bash
$DOCKER run -it -v $(pwd)/ssh:/root/.ssh --name $NAME $IMG bash

}

# destroy
cleanup() {
  echo "cleaning up"
  $DOCKER stop $NAME
  $DOCKER rm $NAME

  rm -r $(pwd)/ssh

}

run_n_enter
cleanup
