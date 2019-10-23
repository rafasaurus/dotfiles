#!/bin/bash
# install i3lock-color
git clone https://github.com/PandorasFox/i3lock-color
cd i3lock-color

autoreconf --force --install
rm -rf build/
mkdir -p build && cd build/

../configure \
  --prefix=/usr \
  --sysconfdir=/etc \
  --disable-sanitizers
sudo make -j4 install

# install betterlockscreen
git clone https://github.com/pavanjadhaw/betterlockscreen
cd betterlockscreen
sudo cp betterlockscreen /bin/

# copy default config script
cp examples/betterlockscreenrc ~/.config
betterlockscreen -u ~/github/config/wallpaper
