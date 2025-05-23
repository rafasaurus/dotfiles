#!/bin/sh
if [ "$XDG_SESSION_TYPE" = "wayland" ]; then 
    setxkbmap -layout 'us'
    swaylock -e -F -k -f -c 000000
else
    setxkbmap -layout 'us'
    xsecurelock
fi
