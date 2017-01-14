#!/bin/bash

# TODO: 
##
## INSTALL OF PACKAGES
### other
# pwgen
# dropbox
# guake/yakuake AND CONFIG
#
### yakuake integration with dolphin
# sudo wget -O /usr/local/bin/yakuake-session https://raw.githubusercontent.com/nextgenthemes/yakuake-session/master/yakuake-session
# sudo apt-get install nautilus-actions
# nautilus-actions-config-tool -> trigger yakuake as in http://askubuntu.com/questions/76712/setting-nautilus-open-terminal-to-launch-terminator-rather-than-gnome-terminal/76747#76747
#-
### Screen on for videos
# sudo apt-get install caffeine
# -> also configure for autostart
#
# eeepc
# kali-linux-full xserver-xorg-input-synaptics
# echo "options ath9k nohwcrypt=1" > /etc/modprobe.d/ath9k.conf


# binaries
load_paths() {
OPENSSL=$(which openssl)
PYTHON=$(which python)
WGET=$(which wget)
}

# constants
USER=$(whoami)
HOSTNAME=$(hostname)
FOLDRHOME="/home/$USER"
FOLDRCONFIG="$FOLDRHOME/.sysangel"
FOLDRROLES="$FOLDRCONFIG/ROLES"
VERSIONFILE="${FOLDRCONFIG}/VERSION.TXT"
CFGFILE="${FOLDRCONFIG}/CONFIG.TXT"
FOLDRKEYS="${FOLDRCONFIG}/KEYS"
PUBK="${FOLDRKEYS}/pub.key"
PRIVK="${FOLDRKEYS}/priv.key"
ENCFSPASS="${FOLDRKEYS}/encfs.pass"
FOLDRDBOX="${FOLDRHOME}/Dropbox"
FOLDRDBOX_ENC="${FOLDRDBOX}/.${USER}.encfs"
FOLDRDBOX_DEC="${FOLDRCONFIG}/${USER}.private"
FOLDRROLESPRIV="${FOLDRDBOX_DEC}/ROLES"
FOLDRBIN=$(pwd)
FOLDRSCRIPTS="$FOLDRBIN/SCRIPTS"

ROLES_system=( desktop server other )
ROLES_desktop=( kde other )
ROLES_user=( sysadmin other )

NEEDED_PACKAGES="git openssl python python-gpgme python-gtk2 wget" 

title_msg() {
  COLUMNS=$(tput cols)
  printf "%*s\n" $(((${#title}+$COLUMNS)/2)) "$1 - (Press a Key to Continue)"

  read cntinue

}

key_generation() {
  echo "FIRST STEP: SECURITY"
  echo 
  echo "We are going to use a key pair to encrypt any password you might need"
  key_needed="true"
  if [ -f ${PRIVK} ]; then
    echo "A private key already exists!"
    echo " Do you want to keep using it? [Y/N] (default Y)"
    read opt
    case $opt in
      n*|N*)
        echo "NO"
        ;;
      y*|Y*|s*|S*|j*|J*|*)
        echo "YES"
        key_needed="false"
        ;;
    esac
  fi

  if ${key_needed}; then
    echo "CREATING A NEW KEY"
    ${OPENSSL} genrsa -out ${PRIVK} 4096 
  fi
  
  #Independently on whether we have it or not, we recreate the public key, to check the private one is OK
  ${OPENSSL} rsa -in ${PRIVK} -pubout > ${PUBK}
}

encfs_paths() {
  while [ "${ENCOK}" != "ok" ]; do
    if [ -d ${FOLDRDBOX_ENC} ]; then
      shopt -s nullglob dotglob; files=(${FOLDRDBOX_ENC}/*)
      if [ ${#files[@]} -gt 0 ]; then
        echo "${FOLDRDBOX_ENC} exists and HAS DATA, do you want to [O]verwrite it, [R]euse it or [C]hoose a different one?"
        while true; do read -r -n 1 -p "${1:-} [o/r/c]: " REPLY
          case $REPLY in
            [oO]) echo ; 
              rm -rf ${FOLDRDBOX_ENC}; mkdir ${FOLDRDBOX_ENC} 
              return 0 ;;
            [rR])  
              #read -r -n 1 -p "Enter the Password for this volume" NEWPASS
              echo; echo -n "Enter the Password for this volume:"
              read -s  NEWPASS; echo
              echo "${NEWPASS}" | openssl rsautl -inkey ${PUBK} -pubin -encrypt > ${ENCFSPASS}
              mount_encfs
              ENCOK=$(cat ${FOLDRDBOX_ENC}/test.io)
              return 0;;
            [cC]) 
							FOLDEROK=1
							while [ ${FOLDEROK} -eq 1 ]; do
								echo; echo -n "Enter the New Encrypted Folder name (under ${FOLDRDBOX}/) - "
								read NEWFOLDRDBOX_ENC; echo
								mkdir ${FOLDRDBOX}/${NEWFOLDRDBOX_ENC}; FOLDEROK=$?
							done; FOLDRDBOX_ENC="${FOLDRDBOX}/${NEWFOLDRDBOX_ENC}"
              return 0 ;;
            *) printf " \033[31m %s \n\033[0m" "invalid input"
          esac 
        done
      else
        ENCOK="ok" 
      fi
    else
      mkdir -p ${FOLDRDBOX_ENC}
      echo "${FOLDRDBOX_ENC} folder created"
      ENCOK="ok"
    fi

    if [ -d ${FOLDRDBOX_DEC} ]; then
			shopt -s nullglob dotglob; files=(${FOLDRDBOX_DEC}/*)
			if [ ${#files[@]} -gt 0 ]; then
				echo "${FOLDRDBOX_DEC} exists and HAS DATA, do you want to [O]verwrite it, or [C]hoose a different one?"
				while true; do read -r -n 1 -p "${1:-} [o/c]: " REPLY
					case $REPLY in
						[oO]) echo ; 
							rm -rf ${FOLDRDBOX_DEC}; mkdir ${FOLDRDBOX_DEC} 
							return 0 ;;
						[cC]) echo ; 
							FOLDEROK=1
							while [ ${FOLDEROK} -eq 1 ]; do
								echo; echo -n "Enter the New Decrypted folder name (under ${FOLDRDBOX}/) - "
								read NEWFOLDRDBOX_DEC; echo
								mkdir ${FOLDRDBOX}/${NEWFOLDRDBOX_DEC}; FOLDEROK=$?
							done; FOLDRDBOX_DEC="${FOLDRDBOX}/${NEWFOLDRDBOX_DEC}"
							return 0 ;;
						*) printf " \033[31m %s \n\033[0m" "invalid input"
					esac 
				done
			fi
		else
			mkdir -p ${FOLDRDBOX_DEC}
			echo "${FOLDRDBOX_DEC} folder created"
		fi
	done

}

config_encfs() {
  echo -n " - Please, indicate a Password for your encrypted folder:"
  read -s encfs_pass
  echo

  expect <<- DONE
    set timeout 5
    spawn encfs ${FOLDRDBOX_ENC} ${FOLDRDBOX_DEC}
    match_max 100000000
    expect  "?>"
    send    "p\n"
    expect  "New Encfs Password:"
    send    "$encfs_pass\n"
    expect  "Verify Encfs Password:"
    send    "$encfs_pass\n"
    expect eof
DONE

  title_msg "writing into the folder"
  echo "ok" > ${FOLDRDBOX_DEC}/test.io

  echo "${encfs_pass}" | openssl rsautl -inkey ${PUBK} -pubin -encrypt > ${ENCFSPASS}

  title_msg "unmounting the volume"
  sudo umount ${FOLDRDBOX_DEC}
}


mount_encfs() {
  encfs_read_pass=$(openssl rsautl -inkey ${PRIVK} -decrypt < ${ENCFSPASS})
  echo ${encfs_read_pass}
  title_msg "mounting it back automatically (no need to type pass!)"
  expect <<- DONE
    set timeout 5
    spawn encfs ${FOLDRDBOX_ENC} ${FOLDRDBOX_DEC}
    match_max 100000000
    expect  "Encfs Password:"
    send    "$encfs_read_pass\n"
    expect eof
DONE

  title_msg "testing the mount"
  cat ${FOLDRDBOX_DEC}/test.io
}


cloudconfig() {
  # Is Dropbox Installed?
  DBOXINSTALLED=$(apt-cache policy dropbox | grep Installed | grep -v "none" | wc -l)
  if [ ${DBOXINSTALLED} -eq 0 ]; then
      sudo apt-key adv --keyserver pgp.mit.edu --recv-keys 5044912E
      sudo apt-get update && sudo apt-get install dropbox
  fi

  # Is the Dropbox Python controller installed?
  if [ ! -f ${FOLDRSCRIPTS}/dropbox.py ]; then
    ${WGET} -O ${FOLDRSCRIPTS}/dropbox.py "https://www.dropbox.com/download?dl=packages/dropbox.py"
  fi

  # Is Dropbox Running?
  DBOXRUNS=$(${PYTHON} ${FOLDRSCRIPTS}/dropbox.py status | grep "Dropbox isn't running" | wc -l)
  if [ ${DBOXRUNS} -eq 1 ]; then
    ${PYTHON} ${FOLDRSCRIPTS}/dropbox.py autostart y
    ${PYTHON} ${FOLDRSCRIPTS}/dropbox.py start -i
  fi
  
  #Is the folder in Dropbox created? if not, create it!
  if [ ! -d ${FOLDRDBOX} ]
  then
    mkdir ${FOLDRDBOX}
  fi

  encfs_paths
  config_encfs
  mount_encfs

}


  # This has to go after the user has configured its own encrypted filesystem
#echo "$PASSPHRASE" | openssl rsautl -inkey $FOLDRCONFIG/public.key -pubin -encrypt > $FOLDRCONFIG/pass.bin
#openssl rsautl -inkey $FOLDRCONFIG/private.key -decrypt < $FOLDRCONFIG/pass.bin

role_selection() {
  # Add roles for this machine
  echo "# Configuration roles for this machine, feel free to modify" > ${FOLDRCONFIG}/${MACHINE}.roles
  echo "#   But bear in mind they will be used in strict order, last overwrites previous" >> ${FOLDRCONFIG}/${MACHINE}.roles
  echo "ROLES:" >> ${FOLDRCONFIG}/${MACHINE}.roles

  # Default role, always added.
  role="common"
  echo "  - ${role}" >> ${FOLDRCONFIG}/${MACHINE}.roles
  ln -s ${FOLDRBIN}/ROLES/${role}.yaml ${FOLDRROLES}/${role}.yaml 2>/dev/null
  echo "    - role ${role} added!"

	# Download the definitions for the first time
	#  and add them to the list
	PS3='Please enter your system type: '
	select opt in "${ROLES_system[@]}"
	do
		case $opt in
			"desktop")
        role=${opt}
        echo "  - ${role}" >> ${FOLDRCONFIG}/${MACHINE}.roles
        ln -s ${FOLDRBIN}/ROLES/${role}.yaml ${FOLDRROLES}/${role}.yaml 2>/dev/null
        echo "    - role ${role} added!"

        PS3='Please enter your desktop: '
				select opt in "${ROLES_desktop[@]}"
				do
					case $opt in
						"kde")
              role=${opt}
              echo "  - ${role}" >> ${FOLDRCONFIG}/${MACHINE}.roles
              ln -s ${FOLDRBIN}/ROLES/${role}.yaml ${FOLDRROLES}/${role}.yaml 2>/dev/null
              echo "    - role ${role} added!"
							echo "you chose choice 1"
              break 2
							;;
						"other")
							break 2
							;;
						*) echo invalid option;;
					esac
				done
				;;
			"server")
        role=${opt}
        echo "  - ${role}" >> ${FOLDRCONFIG}/${MACHINE}.roles
        ln -s ${FOLDRBIN}/ROLES/${role}.yaml ${FOLDRROLES}/${role}.yaml 2>/dev/null
        echo "    - role ${role} added!"
				break
				;;
			"other")
				break
				;;
			*) echo invalid option;;
		esac
	done

	PS3='Please enter your user type: '
	select opt in "${ROLES_user[@]}"
	do
		case $opt in
			"sysadmin")
        role=${opt}
				echo "sysadmin"
        break
				;;
			"other")
				break
				;;
			*) echo invalid option;;
		esac
	done

}

configfile() {
  echo "installdir = "${FOLDRBIN} > ${CFGFILE}

  read -p "- Please, indicate name for this machine ["${HOSTNAME}"] " answer
  if [ "${answer}" = "" ]; then MACHINE=${HOSTNAME}
  else MACHINE=${answer}; fi
  echo "  - From now on we will refer to this machine as ${MACHINE}"

  echo "machinename = "${MACHINE} >> ${CFGFILE}

  role_selection

  echo "  - "${MACHINE} >> ${FOLDRCONFIG}/${MACHINE}.roles
  # For this machine's one we have to be more careful
  echo "- Creating definition for '${MACHINE}'"
  if [ -e "${FOLDRROLESPRIV}/${MACHINE}.yaml" ]; then
    echo "${FOLDRROLESPRIV}/${MACHINE}.yaml exists"
    while true; do
      read -p "Do you want to overwrite?[y/n] " answer
      case $answer in
        [yY]* ) echo "INSTALL:" > ${FOLDRROLESPRIV}/${MACHINE}.yaml
          ln -s ${FOLDRROLESPRIV}/${MACHINE}.yaml ${FOLDRROLES}/${MACHINE}.yaml 2>/dev/null
          break;;
        [nN]* ) break;;
        * )     echo "Dude, just enter Y or N, please.";;
      esac
    done
  else
    echo "INSTALL:" >> ${FOLDRROLESPRIV}/${MACHINE}.yaml
    ln -s ${FOLDRROLESPRIV}/${MACHINE}.yaml ${FOLDRROLES}/${MACHINE}.yaml 2>/dev/null
  fi
  echo >> ${FOLDRROLES}/${MACHINE}.yaml
  
  echo "  DONE"



  # Add some characteristics of this machine
  echo "# Attention! These Facts are not meant to be changed manually" >> ${FOLDRCONFIG}/${MACHINE}.roles
  echo "FACTS:" >> ${FOLDRCONFIG}/${MACHINE}.roles
  # First the Distro and codename
  DISTRO=$(${PYTHON} ${FOLDRBIN}/sysangel.py get-distro)
  DIST=$(echo ${DISTRO} | awk -F "," '/1/ {print $1}')
  CODENAME=$(echo ${DISTRO} | awk -F "," '/1/ {print $3}')
  echo "  distro:" >> ${FOLDRCONFIG}/${MACHINE}.roles
  echo "    "${DIST} >> ${FOLDRCONFIG}/${MACHINE}.roles
  echo "  codename:" >> ${FOLDRCONFIG}/${MACHINE}.roles
  echo "    "${CODENAME} >> ${FOLDRCONFIG}/${MACHINE}.roles

  git ls-remote https://github.com/angelalonso/sysangel | sed -n 2p | cut -f1 > ${VERSIONFILE}

}

presentation() {
  echo "_..~\`  {\\SYSANGEL/}  \`~.._"
  echo "####   Installation   ####"
  echo
}

main() {

  # TODO: function to show ERROR or OK messages
  ## maybe also to ask for input
  title_msg "_..~\`  {\\SYSANGEL/}  \`~.._"


  title_msg "Installing needed packages..."
  sudo apt-get -y install ${NEEDED_PACKAGES}

  title_msg "Loading paths..."
  load_paths

  title_msg "Installing config directory..."
  mkdir -p ${FOLDRCONFIG} &> /dev/null
  mkdir -p ${FOLDRROLES} &>/dev/null
  echo "  DONE"

  title_msg "Generating keys..."
  key_generation

  title_msg "Configuring cloud encrypted folder..."
  cloudconfig
  title_msg "Installing cloud automount Profile.d script..."
  sudo cp ${FOLDRSCRIPTS}/profile_sysangel_priv.sh /etc/profile.d/sysangel_priv.sh
  echo "  DONE"

  title_msg "Configuring files..."
  configfile
  ## TODO:
  ## only do this when it makes sense (cancel? something failed that is needed?)
  title_msg "Installing autorun on startup Profile.d script..."
  sudo cp ${FOLDRSCRIPTS}/profile_sysangel.sh /etc/profile.d/sysangel.sh
  echo "  DONE"
}

main

