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
wget https://raw.githubusercontent.com/rafasaurus/config/master/.Xresources -O $HOME/.Xresources
sudo cp st.desktop /usr/share/applications

# this is used for getting URLs, with dmenu via modkey+l
mkdir -p $HOME/gocode
export GOPATH=$HOME/gocode
go get -u mvdan.cc/xurls/cmd/xurls
sudo cp $HOME/gocode/bin/xurls /usr/bin/
