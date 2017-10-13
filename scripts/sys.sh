#!/usr/bin/env bash
#DOCKER="docker"
# TODO:
# Check why it does not work on linux
# Add "everything else" as a parameter to run on the machine
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
  $DOCKER run -it -v ${AWSDIR}:/root/.aws -v ${SSHDIR}:/root/.ssh -v ${KUBEDIR}:/root/.kube --name $NAME $IMG bash
#$DOCKER run -it -v $SSHDIR:/root/.ssh --name $NAME $IMG bash

}

# destroy
cleanup() {
  echo "cleaning up"
  $DOCKER stop $NAME
  $DOCKER rm $NAME

}

run_n_enter
cleanup
