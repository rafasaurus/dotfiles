#!/bin/sh
xrandr --output eDP-1 --off --output DP-1 --mode 3840x2160 --pos 0x0 --rotate normal --output HDMI-1 --off --output DP-2 --off --output DP-3 --off --output DP-4 --off
[ -f ~/.config/wall.png ] && xwallpaper --zoom ~/.config/wall.png || xwallpaper --zoom ~/.config/default_wall.jpg &
pkill picom
picom -b --experimental-backends --blur-method dual_kawase --blur-strength 1
