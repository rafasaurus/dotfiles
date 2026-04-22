#!/bin/sh
# Toggles git submodule urls from https to ssh or vice versa in .gitmodules
# Skips private-fonts: it requires SSH regardless of mode

FILE=".gitmodules"

[ ! -f "$FILE" ] && exit 1

PRIVATE="rafasaurus/private-fonts"

if grep -q "https://github.com/" "$FILE"; then
    sed -i "\|url.*$PRIVATE|! s|url = https://github.com/|url = git@github.com:|g" "$FILE"
    echo "Changed all to git"
else
    sed -i "\|url.*$PRIVATE|! s|url = git@github.com:|url = https://github.com/|g" "$FILE"
    echo "Changed all to https"
fi

git submodule sync --quiet
