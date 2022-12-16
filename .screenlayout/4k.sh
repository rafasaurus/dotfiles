#!/bin/sh
xrandr --output eDP1 --off --output DP1 --off --output DP2 --off --output DP3 --mode 3840x2160 --pos 0x0 --rotate normal --output DP4 --off --output HDMI1 --off --output VIRTUAL1 --off
[ -f ~/.config/wall.png ] && xwallpaper --zoom ~/.config/wall.png || xwallpaper --zoom ~/.config/default_wall.jpg &
pkill picom
notify-send "external display connected" -u critical
#picom -b --blur-method dual_kawase --blur-strength 1
