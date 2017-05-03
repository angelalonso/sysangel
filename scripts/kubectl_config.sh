#!/usr/bin/env bash

if [ "$(uname -m)" == "i686" ]; then
  VERS="386"
else
  VERS="amd64"
fi

echo $VERS
# curl -LO https://storage.googleapis.com/kubernetes-release/release/$(curl -s https://storage.googleapis.com/kubernetes-release/release/stable.txt)/bin/linux/386/kubectl
#
# chmod +x ./kubectl
# sudo mv ./kubectl /usr/local/bin/kubectl
