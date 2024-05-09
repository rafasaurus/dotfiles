#!/bin/sh
external=$(xrandr | grep -w "connected" | grep -v "primary" | awk '{print $1}')
primary=$(xrandr | grep -w "primary" | awk '{print $1}')

[[ -z $external ]] && (notify-send "external display was not detected" && exit 1)

xrandr --output $primary --off --output $external --mode 3840x2160 --pos 0x0 --rotate normal --output HDMI-1 --off --output DP-2 --off --output DP-3 --off --output DP-4 --off
