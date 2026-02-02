#!/bin/sh
killall -9 dwlb dwlb-status
dwlb -ipc -font "monospace:bold:size=12.5" &
dwlb-status | dwlb -status-stdin all &
