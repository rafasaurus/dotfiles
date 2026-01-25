#!/bin/sh
# Startup script for wayland session

systemctl --user import-environment WAYLAND_DISPLAY XDG_CURRENT_DESKTOP QT_QPA_PLATFORMTHEME XCURSOR_THEME XCURSOR_SIZE &
dbus-update-activation-environment --systemd WAYLAND_DISPLAY XDG_CURRENT_DESKTOP &
dbus-update-activation-environment --systemd --all &
killall pipewire pipewire-pulse wireplumber syncthing wmbubble
pipewire &
pipewire-pulse &
sleep 0.5
wireplumber &
syncthing --no-browser &
dwlb -ipc -font "monospace:bold:size=12.5" &
dwlb-status | dwlb -status-stdin all &
swaybg -i $HOME/.config/wall.png &
wmbubble &
bluetoothctl power on &
autored
xrdb -merge ~/.Xresources &
# wlr-randr --output eDP-1 --mode 1920x1200@120.000000Hz &
mako &
notify-send "Welcome to Rice of rafasaurus" &
