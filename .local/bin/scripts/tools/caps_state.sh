#!/bin/bash
# requires xdotool package: default in Arch Linux
state="$(xset q | grep Caps | awk 'BEGIN{FS=" "} {print  $4}')"
if [ "$state" = "on" ];
then 
    xdotool key Caps_Lock
    setxkbmap -layout 'us' && notify-send "lng english"
    xmodmap ~/.Xmodmap
    notify-send "setting xmodmap and setxkmap"
else
    xdotool key Mode_switch
fi
