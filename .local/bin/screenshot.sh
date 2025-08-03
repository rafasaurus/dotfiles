#!/bin/sh
grim -t png -g "$(slurp)" - | tee "$HOME/Pictures/screenshots/$(date +%Y-%m-%d_%H-%M-%S).png" | wl-copy --type image/png
