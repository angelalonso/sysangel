#!/usr/bin/env python
"""
System configuration manager for my own
"""

import sys


def install(system):
    """
    Manages configuration for new installs
    """
    uninstall(system)
    print system


def update(system):
    """
    Manages configuration for system changes and updates
    """
    print system


def uninstall(system):
    """
    Cleans up configuration after sysangel is no longer needed
    """
    print system


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
