#!/bin/bash
find $HOME/obsidian/ -path '*/.*' -prune -o -name '*.md' | wmenu -i -l 15 | xargs -0 -r -d '\n' mdview.sh
