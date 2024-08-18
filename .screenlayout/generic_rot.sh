#!/bin/sh
# this script is to handle any generic external display that has been connected to the system, with rotation
source $HOME/.screenlayout/common.sh

[[ -z $external ]] && notify-send "external display was NOT detected, nothing to do, bye bye👋" && exit 1
xrandr --output $primary --off --output $external --mode $resolution_external --pos 0x0 --rotate right --output HDMI-1 --off --output DP-2 --off --output DP-3 --off --output DP-4 --off
