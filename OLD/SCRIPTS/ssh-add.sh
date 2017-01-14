#!/bin/sh
export SSH_ASKPASS=/usr/bin/ksshaskpass
ssh-add /home/aaf/.ssh/id_rsa_afonseca </dev/null
ssh-add /home/aaf/.ssh/id_rsa </dev/null
