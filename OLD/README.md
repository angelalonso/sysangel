# DEPRECATED!

I decided to use puppet instead for two reasons:
- Reinventing the wheel helped me learn about python and bash, but rapidly the
  challenges I had to face were no longer interesting from a learning point of
  view.
- The company I work for uses Puppet. This means I can get my machines under
  control while learning (and often debugging) the way Puppet works.

So, as usual, feel free to fork, but this is no longer maintained.



# sysangel

Configuration management for my machines, written in Python

## How it works

### Part 1: Installation

The install Script follows the following steps:
- Install needed packages for the installation process
- Loads some constants (paths to be used during installation)
- Creates the .sysangel folder in your home directory, then subdirectories
- Generates a key pair for encryption of private details
- ...tbd

## Pre-Requisites

openssl
python
python-yaml
xterm


All of them will be installed on the install script!

## Install

git clone https://github.com/angelalonso/sysangel
&& cd ./sysangel && bash ./install.sh <br />

## After Install

Modify the list of roles to use at your will:

- Edit ~/sysangel/<machine_name>.roles
- Add/Remove any role you wish/not
- Modify the order bearing in mind that each role overwrites definitions
  existing on the previous one(s)

  Example:

  Role A.yaml has:<br \> 
  INSTALL:
    - Package A
    - Package B <br \> 

  Role B.yaml has:<br \> 
  INSTALL:
    - Package C
    - A different version of Package B
  
  
  In this case we will install:
  - Package A
  - Role B's version of package B
  - Package C

## To be done:
- Option to uninstall things
  - Packages
  - Not packages
- Better roles definition, "mo' programs!'
- Better first run roles selection
- Better Special cases installations
- More Distros

- Machine/key management, encrypted
