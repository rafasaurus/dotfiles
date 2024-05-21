#!/bin/sh
set -x
if [[ $1 == "" ]] #Where "$1" is the positional argument you want to validate 

then
    echo "no arguments, rotation boolean expected"
    exit 0

fi
if [[ "$1" = true ]]; then 
    $HOME/.screenlayout/4k_rotated.sh && $HOME/.screenlayout/virmon.sh
else
    $HOME/.screenlayout/clear-virmon.sh && $HOME/.screenlayout/4k.sh
fi
[ -f ~/.config/wall.png ] && xwallpaper --zoom ~/.config/wall.png || xwallpaper --zoom ~/.config/default_wall.jpg &
pkill picom
notify-send "external display connected" -u critical
