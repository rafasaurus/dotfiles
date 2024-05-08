#!/bin/sh
$HOME/.screenlayout/4k_rotated.sh # generate with arandr
$HOME/.screenlayout/virmon.sh
[ -f ~/.config/wall.png ] && xwallpaper --zoom ~/.config/wall.png || xwallpaper --zoom ~/.config/default_wall.jpg &
pkill picom
notify-send "external display connected" -u critical
