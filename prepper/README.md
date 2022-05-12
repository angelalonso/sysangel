# How to prepare your Raspberry 

## Burn the image to the microSD
For this I use https://www.balena.io/etcher/

## Enable SSH to the bootloader and boot it
- Insert the microsd card and make sure the partition called "boot" is mounted.
- In ubuntu:  
```
$ touch /media/$USER/boot/ssh  
$ echo 'mypassword' | openssl passwd -6 -stdin # copy the result
$ vim /media/$USER/userconf.txt # paste inside <username>:<the result from the previous command>
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
- Let's make sure your keys dont get in the way when ssh'ing
```
$ ssh -o PasswordAuthentication=yes -o PreferredAuthentications=keyboard-interactive,password -o PubkeyAuthentication=no <user>@<ip>
```
- tweak raspbian  
```
$ sudo raspi-config  
```
- \> Localisation Options > Change Locale > choose the Locales you need, hit OK  
- \> Exit > Reboot  

## Update, upgrade, install the basics
```
$ sudo apt-get update && sudo apt-get upgrade && sudo apt-get install vim git
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
- Add your public key
```
$ mkdir -p $HOME/.ssh && touch $HOME/.ssh/authorized_keys
$ vim $HOME/.ssh/authorized_keys # copy here 
```
- Restart SSH
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

# Make Raspberry connect to LAN through Wi-Fi
Connect your Wifi dongle.  
Read https://www.raspberrypi.org/documentation/configuration/wireless/wireless-cli.md  
```
$ sudo raspi-config  
```
\> Localisation Options > Change Wi-fi Country > Choose yours  
\> Network Options > Wi-Fi > add the name of the WiFi network and the pass  

- 

# Install Rust
Follow the Official Guide at https://www.rust-lang.org/tools/install
