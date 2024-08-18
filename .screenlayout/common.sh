#!/bin/sh
# handy variables for screen scripts

external=$(xrandr | grep -w "connected" | grep -v "primary" | awk '{print $1}')
external_resolution=$(xrandr | grep  -A 1 "^$external " | tail -n 1 | awk '{print $1}')
primary=$(xrandr | grep -w "primary" | awk '{print $1}')
