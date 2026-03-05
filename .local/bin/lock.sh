#!/bin/sh
swaylock -e -F -k -f -c 000000 -i "$(find ~/.config/patterns/ -type f | shuf -n 1)" -s tile
