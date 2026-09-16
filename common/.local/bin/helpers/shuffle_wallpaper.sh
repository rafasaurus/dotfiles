#!/bin/sh
pkill swaybg

DIR="$HOME/.config/patterns"
DEST="$HOME/.config/wall-tile.png"

IMAGE=$(find -L "$DIR" -type f \( -iname "*.png" -o -iname "*.jpg" -o -iname "*.jpeg" -o -iname "*.webp" -o -iname "*.gif" -o -iname "*.bmp" \) | shuf -n 1)

if [ -z "$IMAGE" ]; then
    echo "Error: No matching images found in $DIR." >&2
    exit 1
fi

cp -f "$IMAGE" "$DEST"
swaybg -m tile -i "$DEST" &
