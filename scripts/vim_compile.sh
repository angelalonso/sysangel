#!/usr/bin/env bash
# Installs all parts required for the private mounpoint to work automatically


USR=$(whoami)
HOME="/home/${USR}"
#TODO Config this to go to the dropbox folder instead
GITDIR="${HOME}/Software/Dev/sysangel"
# TODO also install fantasque here

install(){
  echo "compiling vim no longer needed, using apt-get instead (v8.0 already)"
#  sudo apt-get install checkinstall libncurses5-dev \
#    libatk1.0-dev \
#    libcairo2-dev libx11-dev libxpm-dev libxt-dev python-dev \
#    python3-dev ruby-dev lua5.1 lua5.1-dev libperl-dev git
#
#  remove
#
#  cd ~
#  git clone https://github.com/vim/vim.git
#  cd vim
#  ./configure --with-features=huge \
#    --enable-multibyte \
#    --enable-rubyinterp=yes \
#    --enable-pythoninterp=yes \
#    --with-python-config-dir=/usr/lib/python2.7/config-i386-linux-gnu \
#    --enable-python3interp=yes \
#    --with-python3-config-dir=/usr/lib/python3.4/config-3.4m-i386-linux-gnu \
#    --enable-perlinterp=yes \
#    --enable-luainterp=yes \
#    --enable-gui=gtk2 --enable-cscope --prefix=/usr
#  make VIMRUNTIMEDIR=/usr/share/vim/vim80
#  cd ~/vim
#  sudo checkinstall
#
#  sudo update-alternatives --install /usr/bin/editor editor /usr/bin/vim 1
#  sudo update-alternatives --set editor /usr/bin/vim
#  sudo update-alternatives --install /usr/bin/vi vi /usr/bin/vim 1
#  sudo update-alternatives --set vi /usr/bin/vim
  sudo apt-get update
  # vim-gtk has lua, needed for newcomplete

  sudo apt-get install vim-gtk

  # Add vimbrant colorscheme
  mkdir -p ${HOME}/.vim/colors
  wget -O ${HOME}/.vim/colors/vimbrant.vim https://raw.githubusercontent.com/chrishunt/color-schemes/master/thayer/vimbrant.vim


  echo "installing fonts, NOTE: these should also be configure for the terminal"
  # Thanks to https://github.com/belluzj/fantasque-sans
  # I already compiled my own, I'd rather just copy it
  sudo mkdir -p /usr/local/share/fonts/f
  sudo cp ./files/FantasqueSansMono_Regular.ttf /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf
  sudo chmod 644 /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf
  sudo chown root:staff /usr/local/share/fonts/f/FantasqueSansMono_Regular.ttf

}

install_plugins(){
  echo "installing plugins"

  if [[ ! -f ${HOME}/.vimrc.orig ]]; then
    mv ${HOME}/.vimrc ${HOME}/.vimrc.orig 2>/dev/null
  else
    rm ${HOME}/.vimrc 2>/dev/null
  fi
  ln -s ${GITDIR}/files/vimrc_home ${HOME}/.vimrc

  mkdir -p ${HOME}/.vim/autoload ${HOME}/.vim/bundle
  if [ -d "~/.vim/bundle/Vundle.vim" ]; then
    git clone https://github.com/VundleVim/Vundle.vim.git ~/.vim/bundle/Vundle.vim
  fi
  if [ -d "~/.vim/bundle/Vundle.vim" ]; then
    curl -LSso ~/.vim/autoload/pathogen.vim https://tpo.pe/pathogen.vim
  fi

  vim +PluginInstall +qall

}

remove(){
  echo "uninstalling vim"

  sudo rm -rf ${HOME}/.vim 2>/dev/null
  sudo rm -rf ${HOME}/vim 2>/dev/null
  sudo dpkg -r vim 2>/dev/null
  sudo apt-get remove --purge vim vim-runtime gvim vim-common vim-gui-common
  sudo rm /usr/bin/vi 2>/dev/null
  sudo rm /usr/bin/vim 2>/dev/null
  sudo rm /usr/bin/vim.basic 2>/dev/null
  sudo rm /usr/bin/vim.gtk 2>/dev/null
  sudo rm -rf /usr/share/vim 2>/dev/null

}

remove_plugins(){
  echo "removing plugins"
  rm ${HOME}/.vimrc
  if [[ -f ${HOME}/.vimrc.orig ]]; then
    cp ${HOME}/.vimrc.orig ${HOME}/.vimrc
  fi

}

case "$1" in
  install|i|Install|I)
    install
    install_plugins
    ;;
  remove|Remove|r|R|uninstall|u|Uninstall|U)
    remove
    remove_plugins
    ;;
  *)
    echo "ERROR: Syntax is $0 [install|remove|]"
    ;; esac
