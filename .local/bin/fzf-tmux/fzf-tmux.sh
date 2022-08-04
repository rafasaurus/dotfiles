#!/usr/bin/env sh
DIR="${0%/*}"
SELECTED="$(find "$DIR" -maxdepth 1 -type f -exec basename {} \; | sort | grep '^_' | sed 's@\.@ @g' | column -s ',' -t |  fzf -e -i --delimiter _ --with-nth='2..' --prompt="fzf-speed: " --info=default --layout=reverse --tiebreak=index | cut -d ' ' -f1)"
# SELECTED="$(find "$DIR" -maxdepth 1 -type f -exec basename {} \; |  fzf -e -i --delimiter _ --with-nth='2..' --prompt="fzf-speed: " --info=default --layout=reverse --tiebreak=index | cut -d ' ' -f1)"
notify-send "$SELECTED"
$DIR/$SELECTED
eval "${DIR}/${SELECTED},*"
