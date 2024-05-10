#!/bin/sh
[ -f ~/.config/wall.png ] && xwallpaper --zoom ~/.config/wall.png || xwallpaper --zoom ~/.config/default_wall.jpg &

dwmblocks | tee /tmp/dwmblock.log &
xset r rate 300 50 &
cp ~/.cache/zsh/history ~/.cache/zsh/history.bak
unclutter &
sxhkd &			# Bind keys with sxhkd
xrdb ~/.cache/wal/.Xresources # || xrdb ~/.Xresources & # update x database
bluetoothctl power on &
dropbox &
xautolock -time 30 -locker lock.sh -notify 30 -notifier "notify-send --urgency low --expire-time=10000 -- 'Locking screen in 30 seconds'" &
xss-lock lock.sh &
surf hackernews.com &

~/.screenlayout/external.sh true
xrandr --dpi 125
wmbubble &
# LD_LIBRARY_PATH="/opt/gai/lib" shermans_applet &
syncthing --no-browser &
# surf https://rss.bsd.am &
pulseaudio -D
dunst &

# run ollama with with unified memory on intel
#
source $HOME/intel/oneapi/setvars.sh && export ZES_ENABLE_SYSMAN=1 && ollama serve 2>&1 | tee /tmp/ollama.log &

# backup zsh history for good
#
cp ~/.cach/zsh/history ~/.cache/zsh/history.bak
# Watch for updates in todo.txt
ls ~/Dropbox/todo/todo.txt | entr -n -r notify-send "Update in todo list 📔" -u critical &
ls ~/Dropbox/todo/done.txt | entr -n -r ~/.local/bin/vis_life.py &
# Watch for updates in xresources when template of pywal has changed
ls ~/.config/wal/templates/.Xresources | entr -n -r wal -n -i .config/wall.png --backend $(cat $HOME/.config/pywal_backend) &
ls ~/.cache/wal/.Xresources | entr -n -r xrdb ~/.cache/wal/.Xresources &

notify-send "Welcome to Rice of rafasaurus"
