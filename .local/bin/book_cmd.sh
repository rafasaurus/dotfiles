#!/bin/bash
find $HOME/tmp/ $HOME/Dropbox/ -name '*.pdf' -o -name "*.epub" | wmenu -i -l 15 | xargs -0 -r -d '\n' zathura
