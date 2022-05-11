#!/usr/bin/env bash

IPRANGE="192.168.1"
sudo nmap -sP ${IPRANGE}.0/24 | grep -B2 'Raspberry Pi Foundation' | grep ${IPRANGE} | awk '{print $5}'
