#!/bin/bash
stty -ixon # Disable ctrl-s and ctrl-q.
shopt -s autocd #Allows you to cd into directory merely by typing the directory name.
HISTSIZE= HISTFILESIZE=10000 # Infinite history.
HISTFILE=~/.cache/zsh/history

[ -f ~/.fzf.bash ] && source ~/.fzf.bash
[ -f "$HOME/.config/aliasrc" ] && source "$HOME/.config/aliasrc"

source ~/.git-prompt.sh

PS1='\[$(tput bold)\]\[$(tput setaf 1)\][\[$(tput setaf 3)\]\u\[$(tput setaf 2)\]@\[$(tput setaf 4)\]\h \[$(tput setaf 5)\]\W\[$(tput setaf 1)\]]\[$(tput setaf 3)\]\[$(tput setaf 2)\]\[$(tput setaf 4)\]\[$(tput setaf 5)\]\[$(tput setaf 11)\]$(__git_ps1 " (%s)")\[$(tput sgr0)\]\$ '
