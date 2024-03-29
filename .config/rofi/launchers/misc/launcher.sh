#!/usr/bin/env bash

## Author  : Aditya Shakya
## Mail    : adi1090x@gmail.com
## Github  : @adi1090x
## Twitter : @adi1090x

# Available Styles
# >> Created and tested on : rofi 1.6.0-1
#
# blurry	blurry_full		kde_simplemenu		kde_krunner		launchpad
# gnome_do	slingshot		appdrawer			appdrawer_alt	appfolder
# column	row				row_center			screen			row_dock		row_dropdown

theme="launchpad"
dir="$HOME/.config/rofi/launchers/misc"

appdrawer_alt.rasi
appdrawer.rasi
appfolder.rasi
blurry_full.rasi
blurry.rasi
column.rasi
gnome_do.rasi
kde_krunner.rasi
kde_simplemenu.rasi
launcher.sh
launchpad.rasi
row_center.rasi
row_dock.rasi
row_dropdown.rasi
row.rasi
screen.rasi
slingshot.rasi

# comment these lines to disable random style
themes=($(ls -p --hide="launcher.sh" $dir))
theme="${themes[$(( $RANDOM % 16 ))]}"

theme="kde_simplemenu"

rofi -no-lazy-grab -show drun -modi drun -theme $dir/"$theme" 
# # debugging
# notify-send "$(basename $dir)/$theme"
