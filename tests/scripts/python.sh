#!/usr/bin/env bash

sudo apt update && \
  sudo apt install python3 python3-pip

PIP3_PACKS="flake8 pipenv xlrd"

pip3 install ${PIP3_PACKS}

