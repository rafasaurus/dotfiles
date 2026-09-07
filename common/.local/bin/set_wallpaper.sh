#!/bin/sh
pkill swaybg

if [ -f "$HOME/.config/wall-tile.png" ]; then 
    swaybg -m tile -i "$HOME/.config/wall-tile.png" &
else
    swaybg -i "$HOME/.config/wall.png" &
fi
