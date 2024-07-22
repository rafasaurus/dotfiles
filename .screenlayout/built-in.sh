#!/bin/sh
# $HOME/.local/bin/change-dpi 100
$HOME/.screenlayout/clear-virmon.sh
$HOME/.screenlayout/laptop_screen.sh # generate with arandr

[ -f ~/.config/wall.png ] && xwallpaper --zoom ~/.config/wall.png || xwallpaper --zoom ~/.config/default_wall.jpg &
notify-send "built in display connected" -u critical
exit 0
