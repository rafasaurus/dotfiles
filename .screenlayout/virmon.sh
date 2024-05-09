#!/bin/sh
# This script creates three different virtual screens in one 4k monitor
#
height=2160
width=3840
screen="DP-1-1"
virtual_screen="VP-1"

xrandr --setmonitor $virtual_screen-1 2160/0x2160/0+0+1680 $screen
xrandr --setmonitor $virtual_screen-2 2160/0x840/0+0+840 none
xrandr --setmonitor $virtual_screen-3 2160/0x840/0+0+0 none
