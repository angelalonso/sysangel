#!/usr/bin/env python
"""
System configuration manager for my own
"""

import subprocess
import sys


def scriptrun(cmd):
    """
    runs shell scripts
    """
    # TODO: manage errors
    subprocess.Popen([cmd], shell=True).communicate()


def install(system):
    """
    Manages configuration for new installs
    """
    uninstall(system)
    # Install Dropbox, encfs, keys...
    scriptrun('./scripts/ohmyzsh.sh install')
    scriptrun('./scripts/secrets.sh install')


def update(system):
    """
    Manages configuration for system changes and updates
    """


def uninstall(system):
    """
    Cleans up configuration after sysangel is no longer needed
    """
    # Removing required packages
    # scriptrun('./scripts/packages.sh remove')
    # Removing Dropbox, encfs, keys...
    scriptrun('./scripts/secrets.sh remove')
    scriptrun('./scripts/ohmyzsh.sh remove')


if __name__ == '__main__':
    if sys.argv[1] == 'install':
        install(sys.argv[2])
    elif sys.argv[1] == 'update':
        update(sys.argv[2])
    elif sys.argv[1] == 'uninstall':
        uninstall(sys.argv[2])
    else:
        print 'ERROR: parameter not recognized'
        print 'SYNTAX:'
        print 'sysangel.py [install|update|uninstall]'
    # TODO: manage empty parameters
