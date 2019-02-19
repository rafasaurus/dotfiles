#!/bin/sh
BASE_DIR="$HOME"
FONTS_DIR="$BASE_DIR/.fonts"
mkdir -p $FONTS_DIR

# fonts
git clone https://github.com/theleagueof/league-mono/
cp league-mono/ttf/* $FONTS_DIR
rm -rf league-mono
# st
git clone https://github.com/lukesmithxyz/st
sudo make -C st install
rm -rf st
# xresources
wget https://raw.githubusercontent.com/rafasaurus/config/master/dotfiles/.Xresources -O $HOME
