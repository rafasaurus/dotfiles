#!/bin/sh
# Toggles git submodule urls from https to ssh or vice versa in .gitmodules

FILE=".gitmodules"

[ ! -f "$FILE" ] && exit 1

if grep -q "https://github.com/" "$FILE"; then
    sed -i 's|url = https://github.com/|url = git@github.com:|g' "$FILE"
    echo "Changed all to git"
else
    sed -i 's|url = git@github.com:|url = https://github.com/|g' "$FILE"
    echo "Changed all to https"
fi

git submodule sync --quiet
