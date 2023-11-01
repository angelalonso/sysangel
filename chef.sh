#!/usr/bin/env bash

#gem install chef-zero
sudo dpkg -i ./chef-workstation_21.10.640-1_amd64.deb

sudo chef-client --local-mode  ./chef/local.rb


