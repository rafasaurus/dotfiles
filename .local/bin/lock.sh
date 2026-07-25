#!/bin/sh
# swaylock -e -F -k -f -c 000000 -i "$(find ~/.config/patterns/ -type f | shuf -n 1)" -s tile
swaylock -e -F -k -f -c 000000 -i "$(ls ~/.config/patterns/cloud* | shuf -n 1)" -s tile
