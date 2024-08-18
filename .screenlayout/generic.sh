#!/bin/sh
# this script is to handle any generic external display that has been connected to the system
$HOME/.screenlayout/clear-virmon.sh
source $HOME/.screenlayout/common.sh

[[ -z $external ]] && notify-send "external display was NOT detected, nothing to do, bye bye👋" && exit 1

rotate=$(echo "normal left right" | tr ' ' '\n' | xmenu -i -r -p 0x0)
# Do not persued when display mode has not been selected
echo $rotate | grep -E "normal|right|left" || exit 1

echo external=$external
echo primary=$primary
echo res=$resolution_external
xrandr --output $primary --off --output $external --mode $external_resolution --pos 0x0 --rotate $rotate --output HDMI-1 --off --output DP-2 --off --output DP-3 --off --output DP-4 --off
