#!/bin/sh
killall -9 dwlb dwlb-status
pkill -9 -f '[/]pactl-status-watch'
dwlb -ipc -font "Ttyp0 OTB:regular:size=13:antialias=false" &
pactl-status-watch &
dwlb-status | dwlb -status-stdin all &
