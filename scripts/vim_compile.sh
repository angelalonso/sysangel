#!/usr/bin/env bash
# Installs all parts required for the private mounpoint to work automatically

install(){
  echo "compiling vim"
  sudo apt-get install checkinstall libncurses5-dev libgnome2-dev libgnomeui-dev \
    libgtk2.0-dev libatk1.0-dev libbonoboui2-dev \
    libcairo2-dev libx11-dev libxpm-dev libxt-dev python-dev \
    python3-dev ruby-dev lua5.1 lua5.1-dev libperl-dev git

  sudo apt-get remove vim vim-runtime gvim

  cd ~
  git clone https://github.com/vim/vim.git
  cd vim
  ./configure --with-features=huge \
    --enable-multibyte \
    --enable-rubyinterp=yes \
    --enable-pythoninterp=yes \
    --with-python-config-dir=/usr/lib/python2.7/config-i386-linux-gnu \
    --enable-python3interp=yes \
    --with-python3-config-dir=/usr/lib/python3.4/config-3.4m-i386-linux-gnu \
    --enable-perlinterp=yes \
    --enable-luainterp=yes \
    --enable-gui=gtk2 --enable-cscope --prefix=/usr
  make VIMRUNTIMEDIR=/usr/share/vim/vim80
  cd ~/vim
  sudo checkinstall

  sudo update-alternatives --install /usr/bin/editor editor /usr/bin/vim 1
  sudo update-alternatives --set editor /usr/bin/vim
  sudo update-alternatives --install /usr/bin/vi vi /usr/bin/vim 1
  sudo update-alternatives --set vi /usr/bin/vim
}

remove(){
  echo "uninstalling vim"
}

case "$1" in
  install|i|Install|I)
    install
    ;;
  remove|Remove|r|R|uninstall|u|Uninstall|U)
    remove
    ;;
  *)
    echo "ERROR: Syntax is $0 [install|remove|]"
    ;;
esac
