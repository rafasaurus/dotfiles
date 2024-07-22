#!/bin/sh
set -x

# $HOME/.local/bin/change-dpi 96
# TODO, do not rotate when different is plugged, other then 4k
$HOME/.screenlayout/generic.sh
virmon=$(echo "virmon no_virmon" | tr ' ' '\n' | xmenu -i -r -p 0x0)
if [[ $virmon = "virmon" ]]; then
    $HOME/.screenlayout/virmon.sh
fi

[ -f ~/.config/wall.png ] && xwallpaper --zoom ~/.config/wall.png || xwallpaper --zoom ~/.config/default_wall.jpg &

notify-send "switching to external display" -u critical
