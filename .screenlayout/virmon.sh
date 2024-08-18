#!/bin/sh
source $HOME/.screenlayout/common.sh
virtual_screen="VP-1"

notify-send "detected $external_resolution monitor $external"
if [ "$external_resolution" = "3840x2160" ]; then
    xrandr --setmonitor $virtual_screen-1 2160/0x2160/0+0+1680 $external
    xrandr --setmonitor $virtual_screen-2 2160/0x840/0+0+840 none
    xrandr --setmonitor $virtual_screen-3 2160/0x840/0+0+0 none
elif [ "$external_resolution" = "2560x2880" ]; then
    xrandr --setmonitor $virtual_screen-1 2560/0x2060/0+0+820 $external
    xrandr --setmonitor $virtual_screen-2 2560/0x820/0+0+0 none
fi
