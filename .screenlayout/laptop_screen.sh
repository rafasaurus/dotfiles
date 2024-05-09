#!/bin/sh
primary=$(xrandr | grep -w "primary" | awk '{print $1}')

xrandr --output $primary --primary --mode 1920x1200 --pos 0x0 --rotate normal --output DP-1 --off --output HDMI-1 --off --output DP-2 --off --output DP-3 --off --output DP-4 --off --output DP-1-1 --off --output DP-1-2 --off --output DP-1-3 --off
