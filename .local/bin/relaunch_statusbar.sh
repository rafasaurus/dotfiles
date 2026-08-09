#!/bin/sh
killall -9 dwlb dwlb-status
dwlb -ipc -font "Ttyp0 OTB:size=11.5:antialias=false" &
dwlb-status | dwlb -status-stdin all &
