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
sudo apt install -y libx11-dev libxft-dev dmenu golang
sudo make -C st install
rm -rf st
# xresources
wget https://raw.githubusercontent.com/rafasaurus/config/master/dotfiles/.Xresources -O $HOME/.Xresources
if [  -n "$(uname -a | grep Ubuntu)" ]; then
    sudo cp st.desktop /usr/share/applications
fi

mkdir -p $HOME/gocode
export GOPATH=$HOME/gocode
go get -u mvdan.cc/xurls/cmd/xurls
sudo cp $HOME/gocode/bin/xurls /usr/bin/
