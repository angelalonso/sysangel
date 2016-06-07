#!/usr/bin/env python3

import platform
import sys
import yaml


def get_system():
    this_system = platform.dist()
# this_system = platform.system()
    return this_system

def get_distro():
    this_distro = get_system()
    return (this_distro[0] + "," + this_distro[1] + "," + this_distro[2])


def read_config(config_file):
    f = open(config_file)
    dataMap = yaml.safe_load(f)
    print(dataMap)
    f.close()


def main():
    print('Here I am')


if __name__ == '__main__':
    try:
        if (sys.argv[1] == 'get-distro'):
            print(get_distro())
        else:
            print("ERROR: parameter not recognized")
            print("(did you mean sysangel.py get-distro?)")
    except(IndexError):
        config_roles = "/home/aaf/sysangel/zenux.roles"
        read_config(config_roles)
        main()
