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
$ sudo apt-get update && sudo apt-get upgrade && sudo apt-get install vim git unattended-upgrades mailutils
```
- Get your user to be notified
```
$ sudo vim /etc/apt/apt.conf.d/50unattended-upgrades # then uncomment and modify the following:
Unattended-Upgrade::Mail "youruser";
```
- Set a periodic upgrade
```
$ sudo vim /etc/apt/apt.conf.d/02periodic # Then add the following:
APT::Periodic::Enable "1";
APT::Periodic::Update-Package-Lists "1";
APT::Periodic::Download-Upgradeable-Packages "1";
APT::Periodic::Unattended-Upgrade "1";
APT::Periodic::AutocleanInterval "1";
APT::Periodic::Verbose "2";
```
- Test and debug with
```
$ sudo unattended-upgrades -d
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
- Allow pinging 'out'
```
$ sudo vim /etc/ufw/before.rules # and add the following:

# ok icmp codes for OUTPUT
-A ufw-before-output -p icmp --icmp-type destination-unreachable -j ACCEPT
-A ufw-before-output -p icmp --icmp-type time-exceeded -j ACCEPT
-A ufw-before-output -p icmp --icmp-type parameter-problem -j ACCEPT
-A ufw-before-output -p icmp --icmp-type echo-request -j ACCEPT
```
- Configure default config  
```
$ sudo ufw default deny incoming  
$ sudo ufw default deny outgoing  
$ sudo ufw allow ${SSH_PORT}  
$ sudo ufw allow out 53 # DNS
$ sudo ufw allow out 80 # needed at least for apt updates
$ sudo ufw allow out 443 # if 80 is open, we could as well open 443
$ sudo ufw enable  
```

## Kismet
https://github.com/azmatt/chasing_your_tail/blob/main/prereqs.pdf

sudo apt-get update && sudo apt-get upgrade
wget -O - https://www.kismetwireless.net/repos/kismet-release.gpg.key | sudo apt-key add -
echo "deb https://www.kismetwireless.net/repos/apt/release/$(lsb_release -cs) $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/kismet.list
sudo apt-get update
sudo apt-get install kismet
sudo usermod -aG kismet $USER # include your user in the kismet group
sudo reboot

groups # test it worked
------------------ Reviewed until here
Cannot open mailbox /var/mail/freeman: Permission denied

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
