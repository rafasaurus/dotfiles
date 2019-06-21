#!/bin/bash
pkill compton
compton --config ~/.config/compton/config --vsync opengl --mark-wmwin-focused --mark-ovredir-focused -b
