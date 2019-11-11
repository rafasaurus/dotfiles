#!/bin/sh
choices="logout\nshutdown\nsuspend\nlock"

chosen=$(echo -e "$choices" | dmenu -i -p "Choose power")
case "$chosen" in
    logout) i3-msg exit;;
    lock) betterlockscreen --lock blur -r 1280x720;;
    shutdown) ~/.scripts/tools/prompt "Do you want to shutdown ?" shutdown now;;
    suspend) betterlockscreen -s --lock blur;;
esac
