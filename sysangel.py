#!/usr/bin/env python

import platform
import sys
import yaml


''' Functions to take actions '''


def update_ubuntu(config):

    print("Ubuntu not yet implemented")


def update_debian():
    print("Debian not yet implemented")


def update_machine(config):
    current_distro = get_distro()
    system_types = {'Ubuntu': update_ubuntu,
                    'Debian': update_debian,
                    }

    if (current_distro.split(',')[0] == config['FACTS']['distro'] and
            current_distro.split(',')[2] == config['FACTS']['codename']):
        system_types[current_distro.split(',')[0]]()

    else:
        print("ERROR! the Version or even the distro itself has changed!")


''' Functions to get Information '''


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
    f.close()
    return dataMap


''' General Functions '''


def presentation():
    print('####              ####')
    print('#### {\SYSANGEL/} ####')
    print('######################\n\n')


def main(roles_file):
    presentation()
    update_machine(read_config(roles_file))
    bashCommand = "dpkg-query -l vim | grep vim"
    import subprocess
    try:
        output = subprocess.check_output(['bash', '-c', bashCommand])
        print(output)
    except subprocess.CalledProcessError as e:
        print str(e.output)


if __name__ == '__main__':
    roles_file = "/home/aaf/sysangel/zenux.roles"

    try:
        if (sys.argv[1] == 'get-distro'):
            print(get_distro())
        else:
            print("ERROR: parameter not recognized")
            print("(did you mean sysangel.py get-distro?)")
    except(IndexError):
        main(roles_file)
