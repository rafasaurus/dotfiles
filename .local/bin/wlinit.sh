# example https://github.com/FluffyJay1/dots/blob/master/.scripts/wlinit.sh

pipewire & \
pipewire-pulse & \
wireplumber & \
dwlmsg -w > /tmp/dwl_info & \
yambar & \
swaybg -i /home/rafael/.config/wall.png & \
dunst & \
wmbubble & \
dropbox & \
syncthing --no-browser & \
surf hackernews.com & \
bluetoothctl power on & \
notify-send "Welcome to Rice of rafasaurus" & \
/usr/lib/xdg-desktop-portal-gtk & \
/usr/lib/xdg-desktop-portal-wlr & \
wlr-randr --output eDP-1 --mode 1920x1200@120.000000Hz & \
systemctl --user import-environment WAYLAND_DISPLAY XDG_CURRENT_DESKTOP & \
dbus-update-activation-environment --systemd WAYLAND_DISPLAY XDG_CURRENT_DESKTOP & \
dbus-update-activation-environment --systemd --all & \
systemctl --user import-environment QT_QPA_PLATFORMTHEME & \
swayidle -w timeout 300 "lock.sh" before-sleep "setxkbmap -layout 'us'" & \
# /usr/libexec/polkit-gnome-authentication-agent-1 & \
