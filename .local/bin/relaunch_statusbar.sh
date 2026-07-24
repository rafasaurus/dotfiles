#!/bin/sh
killall -9 dwlb dwlb-status
# dwlb -ipc -font "Ttyp0:italic:size=13" &
dwlb -ipc -font "terminus:semibold:size=13:antialias=false" &
dwlb-status | dwlb -status-stdin all &
