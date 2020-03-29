#
# ~/.bash_profile
#

[[ -f ~/.bashrc ]] && . ~/.bashrc

export QT_QPA_PLATFORMTHEME="qt5ct"
export QT_STYLE_OVERRIDE="gtk2"
export QT_SCALE_FACTOR=1.25
export GTK2_RC_FILES="$HOME/.gtkrc-2.0"
export _JAVA_AWT_WM_NONREPARENTING=1 # Fixing misbehaving Java applications
export SUDO_ASKPASS="$HOME/.local/bin/dmenupass"
export BROWSER=chromium

# load colors from wal
[[ -f $HOME/.cache/wal/colors.sh ]] && "$HOME/.cache/wal/colors.sh"

# [[ -f ~/.profile ]] && . ~/.profile
