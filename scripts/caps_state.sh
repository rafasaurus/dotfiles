#!/bin/bash
# requires xdotool package
state="$(xset q | grep Caps | awk 'BEGIN{FS=" "} {print  $4}')"
if [ "$state" = "on" ];
then 
    xdotool key Caps_Lock
    resetXmodmap.sh
    xmodmap.sh
fi
