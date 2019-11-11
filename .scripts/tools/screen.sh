#!/bin/sh
choices="primary\nhdmi\narandr"

chosen=$(echo -e "$choices" | dmenu -i)
case "$chosen" in
    primary) bash ~/github/config/.screenlayout/built-in.sh;;
    hdmi) bash ~/github/config/.screenlayout/1080p_primary.sh;;
    arandr) arandr;;
esac
