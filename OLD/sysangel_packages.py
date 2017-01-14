#!/usr/bin/env python

import filecmp
import logging as log
import os
import subprocess
from shutil import copyfile


''' Logging configuration '''
Logger = log.getLogger()
Logger.setLevel(20)

''' Functions to take actions '''


def install(package, install_type, install_test, yamldata):
    log.debug("Entering install with " + package + " and " + install_type)
    is_installed = check_installed(package, install_type, install_test)

    if (is_installed == 'false'):
        bashRuninstall = str(yamldata['SPECIAL_INSTALL'][package][0]['command'][0])
        log.info("Installing " + package + " with " + bashRuninstall)
        try:
            subprocess.check_output(['bash', '-c', bashRuninstall])
        except subprocess.CalledProcessError as e:
            log.error("Could not install " + package +
                      str(e.output))
    elif (is_installed == 'true'):
        log.debug("Package " + package + " is installed")
    else:
        log.error("Could not check if " + package + " is installed")


def install_w_aptget(package):
    log.debug("Entering install_w_aptget with " + package)
    install_test_apt = "apt-cache policy " + package + " | grep Installed | grep -v none | wc -l"
    is_installed = check_installed(package, "apt", install_test_apt)

    if (is_installed == 'false'):
        bashInstallPackage = 'sudo apt-get -y install ' + package
        try:
            subprocess.check_output(['bash', '-c',
                                    bashInstallPackage],
                                    stderr=subprocess.STDOUT)[0]
            log.info("Installing " + package + " with apt-get")
        except subprocess.CalledProcessError:
            log.error("Could not install " + package)
    elif (is_installed == 'true'):
        log.debug("Package " + package + " is installed")
    else:
        log.error("Could not check if " + package + " is installed")


def uninstall(package, install_type):
    log.debug("Entering uninstall with " + package + " and " + install_type)

    bashUninstallApt = 'sudo apt-get -y remove --purge ' + package
    bashUninstallPip = 'sudo pip uninstall ' + package
    uninstall_commands = []
    is_installed = check_installed(package, install_type, 'none')

    if (install_type == 'apt' and is_installed == 'true'):
        uninstall_commands.append(bashUninstallApt)
    elif (install_type == 'pip' and is_installed == 'true'):
        uninstall_commands.append(bashUninstallPip)
    else:
        if (check_installed(package, 'apt', 'none') == 'true'):
            log.info(package + " is installed with apt")
            uninstall_commands.append(bashUninstallApt)
        if (check_installed(package, 'pip', 'none') == 'true'):
            log.info(package + " is installed with pip")
            uninstall_commands.append(bashUninstallPip)

    for bashUninstall in uninstall_commands:
        try:
            log.info("Uninstalling " + package + " with " + bashUninstall)
            subprocess.check_output(['bash', '-c',
                                    bashUninstall],
                                    stderr=subprocess.STDOUT)[0]
        except subprocess.CalledProcessError:
            log.error("Could not uninstall " + package +
                      " with " + bashUninstall)


def file_action(filename, action, destination):
    log.debug("Entering file_action " + filename + " and " +
              action + " - " + destination)
    if (action == 'copy'):
        # TODO: configl file with bin path
        source = str(os.path.expanduser("~")) + \
                 "/Software/Dev/sysangel/FILES/" + filename
        dest = destination.replace('$HOME', str(os.path.expanduser("~")))
        backupfile = dest + '.sysangel.bck'
        if os.path.isfile(dest):
            if not os.path.isfile(backupfile):
                print "Backup does not exist!"
                copyfile(dest, backupfile)
            if not filecmp.cmp(dest, source):
                print "not the same file, copying"
                copyfile(source, dest)
        else:
            print "copying..."
            copyfile(source, dest)


''' Functions to get Information '''


def check_installed(package, install_type, install_test):
    log.debug("Entering check_installed with " + package +
              " and install_type " + install_type +
              " and install_test " + install_test)

    if (install_type == 'apt'):
        bashis_installed = '/usr/bin/apt-cache policy ' + package + ' | \
                        grep Installed | grep -v "(none)" | wc -l'
    elif (install_type == 'pip'):
        bashis_installed = '/usr/bin/pip list | grep ' + package + ' | wc -l'
    elif (install_type == 'other'):
        if install_test != 'none':
            bashis_installed = install_test
        else:
            bashis_installed = '/usr/bin/which ' + package + ' | \
                                                grep -v "not found"| wc -l'
    else:
        bashis_installed = '/usr/bin/which ' + package + ' | \
                          grep -v "not found"| wc -l'

    try:
        is_installed = subprocess.check_output(['bash', '-c', bashis_installed],
                                          stderr=subprocess.STDOUT)
    except IndexError:
        try:
            is_installed = subprocess.check_output(['bash', '-c', bashis_installed],
                                              stderr=subprocess.STDOUT)[0]
        except subprocess.CalledProcessError as e:
            log.error("Could not check if " + package + " is installed")

    if (int(is_installed) == 0):
        log.debug("Package " + package + " is not installed")
        package_installed = 'false'
    elif (int(is_installed) > 0):
        log.debug("Package " + package + " is installed")
        package_installed = 'true'
    else:
        package_installed = 'error'

    return package_installed
