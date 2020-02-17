#
# ~/.bash_profile
#

[[ -f ~/.bashrc ]] && . ~/.bashrc

export QT_QPA_PLATFORMTHEME="qt5ct"
export GTK2_RC_FILES="$HOME/.gtkrc-2.0"
export _JAVA_AWT_WM_NONREPARENTING=1
export SUDO_ASKPASS="$HOME/.local/bin/dmenupass"
export BROWSER=chromium
# [[ -f ~/.profile ]] && . ~/.profile
