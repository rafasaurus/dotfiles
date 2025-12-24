#!/bin/sh
# example https://github.com/FluffyJay1/dots/blob/master/.scripts/wlinit.sh

systemctl --user import-environment WAYLAND_DISPLAY XDG_CURRENT_DESKTOP QT_QPA_PLATFORMTHEME & \
dbus-update-activation-environment --systemd WAYLAND_DISPLAY XDG_CURRENT_DESKTOP & \
dbus-update-activation-environment --systemd --all & \
pipewire & \
pipewire-pulse & \
wireplumber & \
dwlb -ipc & \
dwlb-status | dwlb -status-stdin all & \
swaybg -i /home/rafael/.config/wall.png & \
dunst & \
syncthing --no-browser & \
surf hackernews.com & \
bluetoothctl power on & \
notify-send "Welcome to Rice of rafasaurus" & \
wlr-randr --output eDP-1 --mode 1920x1200@120.000000Hz & \
# /usr/lib/xdg-desktop-portal-gtk & \
# /usr/lib/xdg-desktop-portal-wlr & \

# /usr/libexec/polkit-gnome-authentication-agent-1 & \
