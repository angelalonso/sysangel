#!/bin/bash

#I had to 
# partition from windows, take note of partitions and recovery keys and whatnot
# burn iso with rufus (UEFI mode)
# disable RAID on BIOS, then enable it again

mkdir -p ~/Software/Dev
mkdir -p ~/.ssh
mkdir -p ~/.kube


sudo apt-get update 
sudo apt-get upgrade
sudo apt-get install \
apt-transport-https \
ca-certificates \
cryfs \
curl \
curl \
dconf-editor \
exfat-fuse \
exfat-utils \
expect \
fabric \
gimp \
git \
git \
htop \
inkscape \
iotop \
jq \
keepassx \
mtr \
nmap \
openssh-client \
passwd \
pwgen \
python \
python-boto3 \
python-pip \
python3 \
python3-pip \
software-properties-common \
tcptraceroute \
terminator \
unzip \
vim \
vim-gtk \
wine-stable \
zip \
zsh \
zsh \
cheese

echo "OH MY ZSH"
sh -c "$(curl -fsSL https://raw.githubusercontent.com/robbyrussell/oh-my-zsh/master/tools/install.sh)"

echo "KUBECTL"
echo " as in https://kubernetes.io/docs/tasks/tools/install-kubectl/"
sudo apt-get update && sudo apt-get install -y apt-transport-https
curl -s https://packages.cloud.google.com/apt/doc/apt-key.gpg | sudo apt-key add -
sudo touch /etc/apt/sources.list.d/kubernetes.list
echo "deb http://apt.kubernetes.io/ kubernetes-xenial main" | sudo tee -a /etc/apt/sources.list.d/kubernetes.list
sudo apt-get update
sudo apt-get install -y kubectl

echo "MINIKUBE"
echo "https://github.com/kubernetes/minikube/releases"
echo "or directly curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.28.2/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/"

echo "FIREFOX"
echo "https://www.mozilla.org/de/firefox/developer/"

echo "VIRTUALBOX"
wget -q -O - http://download.virtualbox.org/virtualbox/debian/oracle_vbox_2016.asc | sudo apt-key add -
sudo sh -c 'echo "deb http://download.virtualbox.org/virtualbox/debian bionic non-free contrib" >> /etc/apt/sources.list.d/virtualbox.org.list' 
sudo apt-get update
sudo apt-get install 
echo "http://download.virtualbox.org/virtualbox/5.1.12/Oracle_VM_VirtualBox_Extension_Pack-5.1.12-112440.vbox-extpack"
echo "https://askubuntu.com/questions/760671/could-not-load-vboxdrv-after-upgrade-to-ubuntu-16-04-and-i-want-to-keep-secur/768310#768310"
echo "openssl req -new -x509 -newkey rsa:2048 -keyout MOK.priv -outform DER -out MOK.der -nodes -days 36500 -subj "/CN=Descriptive common name/""
echo "sudo /usr/src/linux-headers-$(uname -r)/scripts/sign-file sha256 ./MOK.priv ./MOK.der $(modinfo -n vboxdrv)"
echo "ls $(dirname $(modinfo -n vboxdrv))/vbox*.ko"
echo  "repeate sudo /usr/src/linux-headers-$(uname -r)/scripts/sign-file sha256 ./MOK.priv ./MOK.der $(modinfo -n vboxdrv)"
echo "confirm with tail $(modinfo -n vboxdrv) | grep "Module signature appended""
echo "register with sudo mokutil --import MOK.der AND SAVE THE PASSWORD"
echo "Reboot > Enroll MOK > Continue > Enroll? yes > Password, enter it > Reboot"

cd ~ && wget -O - "https://www.dropbox.com/download?plat=lnx.x86_64" | tar xzf -
~/.dropbox-dist/dropboxd


echo "CONFIG:"
echo " - firefox"
echo " - keepassx"
echo " - manually install fantasque font from sysangel repo, files"

cd ~/Software/Dev
git clone https://github.com/angelalonso/sysangel

cd ~
mv .zshrc .zshrc_orig
ln -s ~/Software/Dev/sysangel/files/zshrc_home ~/.zshrc

mv ~/.config/terminator/config ~/.config/terminator/config_orig
ln -s ~/Software/Dev/sysangel/files/terminator_config ~/.config/terminator/config

mv .vimrc .vimrc_orig
ln -s ~/Software/Dev/sysangel/files/vimrc_home ~/.zshrc

mkdir -p ${HOME}/.vim/colors
wget -O ${HOME}/.vim/colors/atom-dark-256.vim https://raw.githubusercontent.com/gosukiwi/vim-atom-dark/master/colors/atom-dark-256.vim

mkdir -p ${HOME}/.vim/autoload ${HOME}/.vim/bundle
if [ -d "~/.vim/bundle/Vundle.vim" ]; then
git clone https://github.com/VundleVim/Vundle.vim.git ~/.vim/bundle/Vundle.vim
fi
if [ -d "~/.vim/bundle/Vundle.vim" ]; then
curl -LSso ~/.vim/autoload/pathogen.vim https://tpo.pe/pathogen.vim
fi

vim +PluginInstall +qall

pip3 install numpy --user
pip3 install pygame --user

echo "DOCKER"
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
udo add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"
sudo apt-get update

echo "TERRAFORM:"
echo "https://www.terraform.io/downloads.html"
sudo apt-get install docker-ce
