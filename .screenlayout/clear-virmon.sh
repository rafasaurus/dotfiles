#!/bin/sh

if [[ $(xrandr --listactivemonitors | grep VP) ]]; then
    for display in $(xrandr --listactivemonitors | awk '{print $2}' | grep VP); do
        echo "killing virtual monitor $display"
        xrandr --delmonitor $display
    done
else 
    echo "nothing to do"
fi
