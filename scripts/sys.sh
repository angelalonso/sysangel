#!/usr/bin/env bash
#DOCKER="docker"
# TODO:
# Check why it does not work on linux
# Add "everything else" as a parameter to run on the machine
DOCKER="sudo docker"
NAME="sysadmin"
IMG="angelalonso/sysadmin:v0.04"


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

  $DOCKER run -it --name $NAME $IMG bash

}

# destroy
cleanup() {
  echo "cleaning up"
  $DOCKER stop $NAME
  $DOCKER rm $NAME
}

sync_run_n_enter() {
  TMPSSH="$(pwd)/ssh"
  TMPAWS="$(pwd)/aws"
  TMPKUBE="$(pwd)/kube"
  rsync -r $SSHDIR/ $TMPSSH
  rsync -r $AWSDIR/ $TMPAWS
  rsync -r $KUBEDIR/ $TMPKUBE

  $DOCKER run -it -v ${TMPAWS}:/root/.aws -v ${TMPSSH}:/root/.ssh -v ${TMPKUBE}:/root/.kube --name $NAME $IMG bash

}

# destroy
sync_n_cleanup() {
  echo "cleaning up"
  $DOCKER stop $NAME
  $DOCKER rm $NAME
  
  # TODO: Use a different user as root, maybe?
  chown -R aaf. $TMPSSH
  chown -R aaf. $TMPAWS
  chown -R aaf. $TMPKUBE

  rsync -r $TMPSSH/ $SSHDIR
  rsync -r $TMPAWS/ $AWSDIR 
  rsync -r $TMPKUBE/ $KUBEDIR  
}

run_n_enter
cleanup
