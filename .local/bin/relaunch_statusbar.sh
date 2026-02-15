#!/bin/sh
killall -9 dwlb dwlb-status
dwlb -ipc -font "monospace:bold:size=11.5" &
dwlb-status | dwlb -status-stdin all &
