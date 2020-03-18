#!/bin/bash
stty -ixon # Disable ctrl-s and ctrl-q.
shopt -s autocd #Allows you to cd into directory merely by typing the directory name.
HISTSIZE= HISTFILESIZE=30000 # Infinite history.
HISTFILE=~/.cache/bash/history

[ -f ~/.fzf.bash ] && source ~/.fzf.bash
[ -f "$HOME/.config/aliasrc" ] && source "$HOME/.config/aliasrc"

prompt_mimir_cmd() {
    if [ $? != 0 ]; then
        local prompt_symbol="\[\e[0;31m\]❯\[\e[0m\]"
    else
        local prompt_symbol="\[\e[0;35m\]❯\[\e[0m\]"
    fi
    PS1="$(mimir)\n${prompt_symbol} "
}
PROMPT_COMMAND=prompt_mimir_cmd

# source ~/.git-prompt.sh
# # 
# PS1='\[$(tput bold)\]\[$(tput setaf 1)\][\[$(tput setaf 3)\]\u\[$(tput setaf 2)\]@\[$(tput setaf 4)\]\h \[$(tput setaf 5)\]\W\[$(tput setaf 1)\]]\[$(tput setaf 3)\]\[$(tput setaf 2)\]\[$(tput setaf 4)\]\[$(tput setaf 5)\]\[$(tput setaf 15)\]$(__git_ps1 "-(%s)")\[$(tput sgr0)\]\$ '
norm="$(printf '\033[0m')" #returns to "normal"
bold="$(printf '\033[0;1m')" #set bold
red="$(printf '\033[0;31m')" #set red
boldyellowonblue="$(printf '\033[0;1;33;44m')" 
boldyellow="$(printf '\033[0;1;33m')"
boldred="$(printf '\033[0;1;31m')" #set bold, and set red.

copython() {
        python $@ 2>&1 | sed -e "s/Traceback/${boldyellowonblue}&${norm}/g" \
        -e "s/File \".*\.py\".*$/${boldyellow}&${norm}/g" \
        -e "s/\, line [[:digit:]]\+/${boldred}&${norm}/g"
    }
neofetch
