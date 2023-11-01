apt_update 'Update the apt cache' do action end

directory 'test' do
  owner 'aaf'
  group 'aaf'
  mode '0755'
  action :create
end

package %w(
apt-transport-https
arp-scan
bat
chromium-browser
cryfs
curl
exa
exfat-fuse
exfatprogs
expect
fabric
gimp
git 
gnupg2
htop
inkscape
iotop
jq
mtr
net-tools
nmap
openssh-client
passwd
pcsxr
pwgen
supertuxkart
tcptraceroute
terminator
unzip
vim 
vim-gtk
vlc
xsel
xterm
zip
zsh
) do
    action :install
end

# variables locally (copy over with chef.sh)
# Use those variables to create Folders
# Secrets, keys...
# oh my zsh https://supermarket.chef.io/cookbooks/lxmx_oh_my_zsh/versions/0.4.1
# Rust
# Python 3.11
# wine
# 
