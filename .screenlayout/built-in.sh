#!/bin/sh
$HOME/.screenlayout/clear-virmon.sh
source $HOME/.screenlayout/common.sh

xrandr --output $primary --primary --mode 1920x1200 --pos 0x0 --rotate normal --output DP-1 --off --output $external --off --output DP-2 --off --output DP-3 --off --output DP-4 --off --output DP-1-1 --off --output DP-1-2 --off --output DP-1-3 --off

[ -f ~/.config/wall.png ] && xwallpaper --zoom ~/.config/wall.png || xwallpaper --zoom ~/.config/default_wall.jpg &
notify-send "built in display connected" -u critical
exit 0
