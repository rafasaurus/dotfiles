#!/bin/sh
# This script deletes all virtual monitor created by virmon.sh
#

xrandr --delmonitor VP-1-1
xrandr --delmonitor VP-1-2
xrandr --delmonitor VP-1-3

# if [[ $(xrandr --listactivemonitors | grep VP) ]]; then
#     for display in $(xrandr --listactivemonitors | awk '{print $2}' | grep VP); do
#         echo "killing virtual monitor $display"
#         xrandr --delmonitor $display
#     done
# else 
#     echo "nothing to do"
# fi
