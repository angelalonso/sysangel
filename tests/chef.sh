#!/usr/bin/env bash

# TODO: 
# - INSTALL ecryptfs first
# - mount external disk
# - get everything from Private
# - 


#sudo chef-client --local-mode  ./chef/local.rb

get_config() {
  echo " - Copying over Config.yaml"
  echo " --------------------------"
  cp ./config.yaml ./config.yaml.bck
  cp ${CONFIG_PATH}/config.yaml .
}

restore_config() {
  echo " - Restoring Config.yaml template"
  echo " --------------------------------"
  mv ./config.yaml.bck ./config.yaml
}

install_chef() {
  echo " - Installing Chef Package"
  echo " -------------------------"
  cp ${CONFIG_PATH}/${CHEF_DEB} .
  sudo dpkg -i ${CHEF_DEB}
  rm ${CHEF_DEB}
}

run_chef() {
  echo " - Running Chef"
  echo " --------------"
  sudo chef-client --local-mode  ./chef/local.rb
}

## VARIABLES
CONFIG_PATH="${HOME}/Private"
CHEF_DEB="chef-workstation_23.4.1032-1_amd64.deb"

get_config
#install_chef
run_chef
restore_config
