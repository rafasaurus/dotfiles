#!/bin/sh
killall -9 dwlb dwlb-status
dwlb -ipc -font "Ttyp0 OTB:regular:size=13:antialias=false" &
dwlb-status | dwlb -status-stdin all &
