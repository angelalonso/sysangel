#!/usr/bin/env bash

#gem install chef-zero
# TODO: COPY config.yaml FROM BACKUP - Private
# TODO: Set a template config.yaml here
# TODO: MOVE .deb to Private, copy over
# TODO: MAYBE INSTALL ecryptfs first??
sudo dpkg -i ./chef-workstation_23.4.1032-1_amd64.deb

sudo chef-client --local-mode  ./chef/local.rb


