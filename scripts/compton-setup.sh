#!/usr/bin/bash

sudo apt install -y libxcomposite-dev libxdamage-dev libxrandr-dev libxinerama-dev libconfig-dev libdbus-1-dev libglx0 libglu1-mesa-dev freeglut3-dev mesa-common-dev
git clone https://github.com/chjj/compton
cd compton 
sudo make install -j4
mkdir -p ~/.scripts 
cp compton.sh ~/.scripts
