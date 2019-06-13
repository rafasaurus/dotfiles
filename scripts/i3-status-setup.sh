#!/usr/bin/bash

sudo apt-get install -y libconfuse-dev libyajl-dev libasound2-dev libiw-dev asciidoc libpulse-dev libnl-genl-3-dev
git clone https://github.com/i3/i3status
cd i3status
autoreconf -fi
mkdir build
cd build
../configure
make -j8
sudo make install -j4
