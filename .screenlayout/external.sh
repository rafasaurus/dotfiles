#!/bin/sh
# this script should be the main interface from user perspective

# TODO, do not rotate when different is plugged, other then 4k
$HOME/.screenlayout/generic.sh || exit 1
virmon=$(echo "virmon no_virmon" | tr ' ' '\n' | xmenu -i -r -p 0x0)
if [[ $virmon = "virmon" ]]; then
    $HOME/.screenlayout/virmon.sh
fi

[ -f ~/.config/wall.png ] && xwallpaper --zoom ~/.config/wall.png || xwallpaper --zoom ~/.config/default_wall.jpg &

notify-send "switching to external display" -u critical
