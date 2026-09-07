#!/bin/bash
stty -ixon # Disable ctrl-s and ctrl-q.
shopt -s autocd #Allows you to cd into directory merely by typing the directory name.
HISTSIZE= HISTFILESIZE=140000
HISTFILE=~/.cache/bash/history

[ -f "$HOME/.aliasrc" ] && source "$HOME/.aliasrc"

prompt_mimir_cmd() {
    if command -v mimir >/dev/null 2>&1; then
        mimir
    else
        echo  "[$PWD]"
    fi
}
PROMPT_COMMAND=prompt_mimir_cmd
PS1="\[\e[35m\]❯\[\e[m\] "
