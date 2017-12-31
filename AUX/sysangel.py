#!/usr/bin/env python

import logging as log
import os
import platform
import re
import subprocess
import sys
import yaml

import sysangel_packages as pkg


''' Logging configuration '''
Logger = log.getLogger()
Logger.setLevel(20)

''' Functions to take actions '''


def update_machine(config_folder, roledefs):
    log.debug("Entering update_machine with " + str(roledefs))

    yamldata = {}

    for role in roledefs['ROLES']:
        yamldata = read_yaml(config_folder, role, yamldata)

    for package in yamldata['INSTALL']:
        current_distro = get_distro()
        system_types = {'Ubuntu': pkg.install_w_aptget,
                        'Debian': pkg.install_w_aptget,
                        }

        if (current_distro.split(',')[0] == roledefs['FACTS']['distro'] and
                current_distro.split(',')[2] == roledefs['FACTS']['codename']):
            system_types[current_distro.split(',')[0]](package)
        else:
            log.error("The Version or even the distro itself has changed!")

    for package in yamldata['SPECIAL_INSTALL']:
        try:
            install_type = str(yamldata['SPECIAL_INSTALL'][package][1]['type'][0])
        except IndexError:
            install_type = 'other'

        try:
            install_test = str(yamldata['SPECIAL_INSTALL'][package][2]['checktest'][0])
        except IndexError:
            install_test = 'none'

        pkg.install(package, install_type, install_test, yamldata)

    for package in yamldata['UNINSTALL']:
        try:
            install_type = str(yamldata['UNINSTALL'][package][0]['type'][0])
        except TypeError:
            install_type = 'other'
        except IndexError:
            install_type = 'other'

        pkg.uninstall(package, install_type)

    for filename in yamldata['FILE']:
        # TODO: the index should not be needed, I am doing something wrong here!
        try:
            action = str(yamldata['FILE'][filename][0]['action'][0])
        except TypeError:
            action = 'other'
        except IndexError:
            action = 'other'

        try:
            destination = str(yamldata['FILE'][filename][1]['destination'][0])
        except TypeError:
            destination = 'other'
        except IndexError:
            destination = 'other'

        pkg.file_action(filename, action, destination)

''' Functions to get Information '''


def get_system():
    log.debug("Entering get_system")

    this_system = platform.dist()

    return this_system


def get_distro():
    log.debug("Entering get_distro")

    this_distro = get_system()

    return (this_distro[0] + "," + this_distro[1] + "," + this_distro[2])


def read_yaml(config_folder, role, yamldata):
    log.debug("Entering read_yaml with " + role + " and " + str(yamldata))

    yaml_file = config_folder + "/ROLES/" + str(role) + ".yaml"
    yf = open(yaml_file)
    dataMap = yaml.safe_load(yf)

    mergedict = mergeinto_yamldict(yamldata, dataMap)

    return mergedict


def read_roles(roles_file):
    log.debug("Entering read_roles with " + roles_file)

    try:
        rf = open(roles_file)
        dataMap = yaml.safe_load(rf)
        rf.close()
        return dataMap
    except IOError:
        log.error("File " + roles_file + " not found!")
        sys.exit(1)

''' General Tools '''


def mergeinto_yamldict(main, secondary):
    log.debug("Entering mergeinto_yamldict with " + str(main) +
              " and " + str(secondary))

    result = {}
    INSTALL = []
    SPECIAL_INSTALL = {}
    UNINSTALL = {}
    FILE = {}

    for entry in main:
        if (entry == 'INSTALL'):
            INSTALL += main[entry]
        elif (entry == 'SPECIAL_INSTALL'):
            SPECIAL_INSTALL = main[entry]
        elif (entry == 'UNINSTALL'):
            UNINSTALL = main[entry]
        elif (entry == 'FILE'):
            FILE = main[entry]

    try:
        if 'INSTALL' in secondary:
            try:
                for entry in secondary['INSTALL']:
                    if entry not in INSTALL:
                        INSTALL.append(entry)
            except TypeError:
                pass
    except TypeError:
        pass

    result['INSTALL'] = INSTALL

    try:
        if 'SPECIAL_INSTALL' in secondary:
            try:
                for entry in secondary['SPECIAL_INSTALL']:
                    try:
                        for key, value in entry.iteritems():
                            SPECIAL_INSTALL[key] = value
                    except TypeError:
                        pass
            except TypeError:
                pass
    except TypeError:
        pass

    result['SPECIAL_INSTALL'] = SPECIAL_INSTALL

    try:
        if 'UNINSTALL' in secondary:
            try:
                for entry in secondary['UNINSTALL']:
                    try:
                        for key, value in entry.iteritems():
                            UNINSTALL[key] = value
                    except AttributeError:
                        pass
                    except TypeError:
                        pass
            except TypeError:
                pass
    except TypeError:
        pass

    result['UNINSTALL'] = UNINSTALL

    try:
        if 'FILE' in secondary:
            try:
                for entry in secondary['FILE']:
                    try:
                        for key, value in entry.iteritems():
                            FILE[key] = value
                    except AttributeError:
                        pass
                    except TypeError:
                        pass
            except TypeError:
                pass
    except TypeError:
        pass

    result['FILE'] = FILE

    return result

''' General Functions '''


def update_self(git_folder, config_folder, version_file, roles_file):
    # This should use the new git checkout, or even git pull
    log.debug("Entering update_self with " + git_folder +
              " and " + version_file + " and " + roles_file)

    with open(version_file) as f:
            installed_version = f.readline()

    bashGetGitVersion = "cd " + git_folder + " && git rev-parse HEAD"

    try:
        current_version = subprocess.check_output(
                ['bash', '-c', bashGetGitVersion],
                stderr=subprocess.STDOUT)

        if ("fatal: unable to access" in current_version):
            log.error("No connection to GIT: definitions remain outdated\n")
        else:
            if (installed_version != current_version):
                bashUpdateGit = "cd " + git_folder + " && git pull"

                try:
                    subprocess.check_output(['bash', '-c', bashUpdateGit])
                    version_buffer = open(version_file, "w")
                    version_buffer.write(current_version)
                    version_buffer.close()
                except subprocess.CalledProcessError as e:
                    log.error("Could not update Git Folder: " +
                          str(e.output))


    except subprocess.CalledProcessError as e:
        log.error("Could not update: " + str(e.output))


def presentation():
    print("_..~`  {\SYSANGEL/}  `~.._\n")


def main(config_folder, roles_file):
    log.debug("Entering main with " + roles_file)

    update_machine(config_folder, read_roles(roles_file))


if __name__ == '__main__':

    config_folder = str(os.path.expanduser("~")) + "/.sysangel"
    configfile = open(config_folder + "/CONFIG.TXT", "r")
    for line in configfile:
        if re.search("installdir = ", line):
            git_folder = line.replace("installdir = ", "").replace("\n", "")
        if re.search("machinename = ", line):
            machinename = line.replace("machinename = ", "").replace("\n", "")
    version_file = config_folder + "/VERSION.TXT"
    roles_file = config_folder + "/" + machinename + ".roles"

    try:
        if (sys.argv[1] == 'get-distro'):
            print(get_distro())
        else:
            log.error("Parameter not recognized \
                      (did you mean sysangel.py get-distro?)")
    except(IndexError):
        presentation()
        update_self(git_folder, config_folder, version_file, roles_file)
        main(config_folder, roles_file)
