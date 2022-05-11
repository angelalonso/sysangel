# How to prepare your Raspberry 

## Burn the image to the microSD
For this I use https://www.balena.io/etcher/

## Enable SSH to the bootloader and boot it
- Insert the microsd card and make sure the partition called "boot" is mounted.
- In ubuntu:  
```
$ touch /media/$USER/boot/ssh  
```
- Unmount the MicroSD card  
- Insert it to the raspberry pi  
- Connect it to power and the network (use a cable, mate!)  
- Find the IP  
```
$ nmap -sP 192.168.1.0/24 # Modify the range to your needs
$ ./find_my_raspberry.sh # Script that does the same, also adapt the IP range
```

## SSH into it, give it a name, tweak raspbian
```
$ ssh ubuntu@\<IP\> # password is ubuntu, accept authenticity
```
- once in, change hostname to your liking  
```
$ sudo hostname \$HOSTNAME  
$ sudo vim /etc/hosts # change raspberrypi to \$HOSTNAME  
$ sudo vim /etc/hostname # change raspberrypi to \$HOSTNAME  
```
- tweak raspbian  
```
$ sudo raspi-config  
```
- \> Localisation Options > Change Locale > choose the Locales you need, hit OK  
- \> Exit > Reboot  

## Add your own admin user, remove user pi
```
$ sudo useradd -s /bin/bash -m -d /home/$NEWUSER $NEWUSER
```
- Give that user a strong password  
```
$ sudo passwd $NEWUSER
```
- Add your SSH key to log in  
```
$ sudo mkdir -p /home/$NEWUSER/.ssh  
$ sudo vi /home/$NEWUSER/.ssh/authorized_keys # Here you should paste your public SSH key and save
```
- Add your user to the same groups as pi is in  
```
$ vigr
```
- (This can probably be achieved in some other way) Let you user manage GPIO
```
$ sudo adduser $NEWUSER dialout gpio
```
```
:%s/:ubuntu/:ubuntu,$NEWUSER/g
```
- Make your NEW USER owner of its own environment  
```
$ sudo chmod 600 /home/$NEWUSER/.ssh/authorized_keys  
$ sudo chown -R $NEWUSER:$NEWUSER /home/$NEWUSER 
```
- Log out, log in again
```
$ logout  
$ ssh -i <PATH TO YOUR SSH KEY> $NEWUSER@<IP>  
```
- Get rid of pi  
```
$ sudo deluser -remove-home ubuntu
```
  
## Update, upgrade, install the basics
```
$ sudo apt-get update  
$ sudo apt-get upgrade  
$ pip install flatdict flask
```
  
## Strengthen SSH
```
$ sudo vim /etc/ssh/sshd_config   
```
```ChallengeResponseAuthentication no
PasswordAuthentication no  
UsePAM no  
PermitRootLogin no  
Port $NEWPORT  
```
```
$ sudo systemctl reload ssh  
```
- Install fail2ban  
```
$ sudo apt-get install fail2ban  
$ sudo cp /etc/fail2ban/jail.conf /etc/fail2ban/jail.local  
$ sudo systemctl restart fail2ban  
```

## Installing and configuring Firewall
- Install and configure UFW  
```
$ sudo apt-get update && sudo apt-get install ufw  
```
- Add support for IPv6  
```
$ sudo vim /etc/default/ufw    
```
```
IPV6=yes  
```  
- Configure default config  
```
$ sudo ufw default deny incoming  
$ sudo ufw default allow outgoing  
$ sudo ufw allow ${SSH_PORT}  
$ sudo ufw enable  
```


# Install some further packages

```
$ sudo apt-get update  
$ sudo apt-get install vim git rpi.gpio-common python3-pigpio pigpio-tools # we'll need them later
$ sudo reboot
```

# Make Raspberry connect to LAN through Wi-Fi
Connect your Wifi dongle.  
Read https://www.raspberrypi.org/documentation/configuration/wireless/wireless-cli.md  
```
$ sudo raspi-config  
```
\> Localisation Options > Change Wi-fi Country > Choose yours  
\> Network Options > Wi-Fi > add the name of the WiFi network and the pass  

# Make pigpio daemon run at boot
- Copy files/pigpiod.service to /etc/systemd/system/
```
$ sudo systemctl enable pigpiod
$ sudo systemctl start pigpiod
```
- 

# Install ROS2

Follow the Official Guide at https://docs.ros.org/en/galactic/Installation.html

# Install Rust
Follow the Official Guide at https://www.rust-lang.org/tools/install
