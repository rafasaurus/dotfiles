#!/bin/sh
xrandr --output eDP1 --mode 1920x1200 --pos 0x0 --rotate normal --output DP1 --off --output DP2 --off --output DP3 --off --output DP4 --off --output HDMI1 --off --output VIRTUAL1 --off
[ -f ~/.config/wall.png ] && xwallpaper --zoom ~/.config/wall.png || xwallpaper --zoom ~/.config/default_wall.jpg &
pkill picom
picom -b --blur-method dual_kawase --blur-strength 5
notify-send "built in display connected" -u critical
