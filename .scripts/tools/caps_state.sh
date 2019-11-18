#!/bin/bash
# requires xdotool package: default in Arch Linux
state="$(xset q | grep Caps | awk 'BEGIN{FS=" "} {print  $4}')"
if [ "$state" = "on" ];
then 
    xdotool key Caps_Lock
    resetXmodmap.sh
    # ~/.scripts/setxkmap.sh
    setxkbmap -layout 'us' && xset r rate 240 50
    xmodmap.sh
    echo "setting xmodmap and setxkmap"
fi
