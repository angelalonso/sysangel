#!/bin/bash

set -eou pipefail


if [ "$#" -ne 1 ]
then
  echo "Usage: ..."
  exit 1
else
  case $1 in
    remove)
      ;;
    packages)
      ;;
    *)
      ;;
  esac

fi

