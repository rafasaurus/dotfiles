#!/usr/bin/env bash

BASE_DIR="$HOME/.vim"
PLUGIN_DIR="$BASE_DIR/plugged"
COLORS_DIR="$BASE_DIR/colors"
AUTOLOAD_DIR="$BASE_DIR/autoload"

mkdir -p $PLUGIN_DIR
mkdir -p $COLORS_DIR
mkdir -p $AUTOLOAD_DIR

wget https://raw.githubusercontent.com/rafasaurus/config/master/dotfiles/.vimrc -O $HOME/.vimrc
wget https://raw.githubusercontent.com/dracula/vim/master/colors/dracula.vim -O $HOME/.vim/colors/dracula.vim 

curl -fLo $AUTOLOAD_DIR/plug.vim --create-dirs \
    https://raw.githubusercontent.com/junegunn/vim-plug/master/plug.vim

vim -c PlugInstall -c q -c q
echo "vim setup done boi"
