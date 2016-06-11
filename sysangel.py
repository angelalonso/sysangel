#!/usr/bin/env python

import platform
import subprocess
import sys
import yaml


''' Functions to take actions '''


def update_ubuntu(roledefs):
    yamldata = {}
    for role in roledefs['ROLES']:
        yamldata = read_yaml(role, yamldata)

    for package in yamldata['INSTALL']:
        print("Installing " + str(package))
        bashCommand = 'if [ $(/usr/bin/apt-cache policy ' + package + ' | \
                        grep Installed | grep -v "(none)" \
                        | wc -l) -ne 1 ];\
                        then sudo apt-get install ' + package + '; \
                        fi'
        try:
            subprocess.check_output(['bash', '-c', bashCommand])
        except subprocess.CalledProcessError as e:
            print "ERROR installing " + package
            print str(e.output)

    for package in yamldata['SPECIAL_INSTALL']:
        print("Installing " + str(package))
        print("Installing " + str(yamldata['SPECIAL_INSTALL'][package]))


def update_debian(roledefs):
    print("Debian not yet implemented")


def update_machine(roledefs):
    current_distro = get_distro()
    system_types = {'Ubuntu': update_ubuntu,
                    'Debian': update_debian,
                    }

    if (current_distro.split(',')[0] == roledefs['FACTS']['distro'] and
            current_distro.split(',')[2] == roledefs['FACTS']['codename']):
        system_types[current_distro.split(',')[0]](roledefs)

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


def read_yaml(role, yamldata):
    yaml_file = "/home/aaf/sysangel/ROLES/" + str(role) + ".yaml"
    yf = open(yaml_file)
    dataMap = yaml.safe_load(yf)
    mergedict = mergeinto_yamldict(yamldata, dataMap)
    return mergedict


def read_roles(roles_file):
    rf = open(roles_file)
    dataMap = yaml.safe_load(rf)
    rf.close()
    return dataMap


''' General Tools '''


def mergeinto_yamldict(main, secondary):
    result = {}
    INSTALL = []
    SPECIAL_INSTALL = {}

    for entry in main:
        if (entry == 'INSTALL'):
            INSTALL += main[entry]
        elif (entry == 'SPECIAL_INSTALL'):
            SPECIAL_INSTALL = main[entry]

    if 'INSTALL' in secondary:
        for entry in secondary['INSTALL']:
            if entry not in INSTALL:
                INSTALL.append(entry)
    result['INSTALL'] = INSTALL

    if 'SPECIAL_INSTALL' in secondary:
        for entry in secondary['SPECIAL_INSTALL']:
            for key, value in entry.iteritems():
                SPECIAL_INSTALL[key] = value
    result['SPECIAL_INSTALL'] = SPECIAL_INSTALL

    return result

''' General Functions '''


def presentation():
    print('####              ####')
    print('#### {\SYSANGEL/} ####')
    print('######################\n\n')


def main(roles_file):
    presentation()
    update_machine(read_roles(roles_file))
    bashCommand = "dpkg-query -l vim | grep vim"
    import subprocess
    try:
        output = subprocess.check_output(['bash', '-c', bashCommand])
        #print(output)
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
