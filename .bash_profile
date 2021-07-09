#
# ~/.bash_profile
#

# [[ -f ~/.bashrc ]] && . ~/.bashrc

export QT_QPA_PLATFORMTHEME="qt5ct"
export QT_STYLE_OVERRIDE="gtk2"
export QT_SCALE_FACTOR=1.25
export GTK2_RC_FILES="$HOME/.gtkrc-2.0"
export _JAVA_AWT_WM_NONREPARENTING=1 # Fixing misbehaving Java applications
export XDG_SESSION_TYPE=X11
export XDG_SESSION_CLASS=user
export XDG_MENU_PREFIX=lxde-
export XDG_SESSION_DESKTOP="dwm"
export XDG_CURRENT_DESKTOP="dwm"
export XDG_CONFIG_HOME="$HOME/.config"
export XDG_DATA_DIRS="/usr/local/share:/usr/share"
export XDG_CONFIG_DIRS="/usr/share/upstart/xdg:/etc/xdg"
export XDG_CACHE_HOME="$HOME/.cache"
export XDG_DATA_HOME="$HOME/.local/share/"
export XDG_RUNTIME_DIR="$HOME/.cache/xdgr/"
export DESKTOP_SESSION="dwm"
export SUDO_ASKPASS="$HOME/.local/bin/dmenupass"
export BROWSER=chromium
export VISUAL="/usr/bin/vim" # for crontab -e
export EDITOR=vim 
export TERMINAL=alacritty
export BROWSER=firefox
# [[ -f ~/.profile ]] && . ~/.profile
