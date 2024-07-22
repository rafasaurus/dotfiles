#!/bin/sh
#
# external monitor
screen=$(xrandr | grep -w "connected" | grep -v "primary" | awk '{print $1}')
virtual_screen="VP-1"
# external monitor resolution
resolution=$(xrandr | grep  -A 1 "^$screen " | tail -n 1 | awk '{print $1}')

notify-send "detected $resolution monitor $screen"
if [ "$resolution" = "3840x2160" ]; then
    xrandr --setmonitor $virtual_screen-1 2160/0x2160/0+0+1680 $screen
    xrandr --setmonitor $virtual_screen-2 2160/0x840/0+0+840 none
    xrandr --setmonitor $virtual_screen-3 2160/0x840/0+0+0 none
elif [ "$resolution" = "2560x2880" ]; then
    xrandr --setmonitor $virtual_screen-1 2560/0x2060/0+0+820 $screen
    xrandr --setmonitor $virtual_screen-2 2560/0x820/0+0+0 none
fi
