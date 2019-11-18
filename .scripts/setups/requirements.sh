#!/bin/sh
sudo pacman -Syu
sudo pacman -S feh rofi xclip xorg-backlight smartmontools pactl firefox maim
yay betterlockscreen
systemctl enable betterlockscreen@$USER
sudo pip3 install wpm
