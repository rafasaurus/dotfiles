#!/bin/sh
case "$1" in
  -h|--help) echo "Usage: $(basename "$0") [-s|--select]"; exit;;
  -s|--select) S="$(slurp)" || exit; set -- -g "$S";;
esac
D="$HOME/Pictures/screenshots"
mkdir -p "$D"
F="$D/$(date +%F_%H-%M-%S).png"
grim -t png "$@" - | tee "$F" | wl-copy --type image/png
