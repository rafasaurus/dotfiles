#!/bin/sh
set -x

# $HOME/.local/bin/change-dpi 96
#
$HOME/.screenlayout/clear-virmon.sh

external=$(xrandr | grep -w "connected" | grep -v "primary" | awk '{print $1}')
primary=$(xrandr | grep -w "primary" | awk '{print $1}')
resolution_external=$(xrandr | grep  -A 1 "^$external " | tail -n 1 | awk '{print $1}')

[[ -z $external ]] && (notify-send "external display was not detected" && exit 1)

rotate=$(echo "normal left right" | tr ' ' '\n' | xmenu -i -r -p 0x0)
echo external=$external
echo primary=$primary
echo res=$resolution_external
xrandr --output $primary --off --output $external --mode $resolution_external --pos 0x0 --rotate $rotate --output HDMI-1 --off --output DP-2 --off --output DP-3 --off --output DP-4 --off
