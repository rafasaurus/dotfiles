autoload -U colors && colors
stty -ixon # Disable ctrl-s and ctrl-q.
setopt autocd #Allows you to cd into directory merely by typing the directory name.

# History in cache directory:
HISTSIZE=140000
SAVEHIST=140000
HISTFILE=~/.cache/zsh/history
setopt    APPEND_HISTORY     #Append history to the history file (no overwriting)
setopt    SHARE_HISTORY      #Share history across terminals
setopt    INC_APPEND_HISTORY  #Immediately append to the history file, not just when a term is killed

# Basic auto/tab complete:
autoload -U compinit
zstyle ':completion:*' menu select
zmodload zsh/complist
compinit
_comp_options+=(globdots)		# Include hidden files.

# vi mode
bindkey -v
export KEYTIMEOUT=1
bindkey -M viins '^G' vi-cmd-mode
bindkey -s -M viins '\e' ''
bindkey -M viins '\ed' kill-word

# Restore "Bash-style" behavior in Insert Mode
bindkey -M viins '^W' backward-kill-word    # Delete word
bindkey -M viins '^U' backward-kill-line    # Delete back to start of line
bindkey -M viins '^K' kill-line             # Delete forward to end of line
bindkey -M viins '^A' beginning-of-line     # Move to start
bindkey -M viins '^E' end-of-line           # Move to end
bindkey -M viins '^P' up-line-or-history    # Previous command
bindkey -M viins '^N' down-line-or-history  # Next command
bindkey -M viins '^Y' yank                  # Paste deleted text

# Use vim keys in tab complete menu:
bindkey -M menuselect 'h' vi-backward-char
bindkey -M menuselect 'k' vi-up-line-or-history
bindkey -M menuselect 'l' vi-forward-char
bindkey -M menuselect 'j' vi-down-line-or-history
bindkey -v '^?' backward-delete-char

autoload -U edit-command-line
zle -N edit-command-line # Edit line in vim with ctrl-e:
bindkey '^E' end-of-line # Move to the end of the line
bindkey -M viins '^Xe' edit-command-line # hold Ctrl, tap X, release both, tap e
bindkey '^A' beginning-of-line # Move to the start of the line
bindkey "^[f" forward-word # Move forward one word
bindkey "^[b" backward-word # Move backward one word

# Change cursor shape for different vi modes.
[ -f /usr/share/fzf/key-bindings.zsh ] && source /usr/share/fzf/key-bindings.zsh

function zle-keymap-select {
  # RPS1="${${KEYMAP/vicmd/-- NORMAL --}/(main|viins)/ -- INSERT --}"
  # RPS2=$RPS1
  zle reset-prompt
  if [[ ${KEYMAP} == vicmd ]] ||
     [[ $1 = 'block' ]]; then
    echo -ne '\e[1 q'
  elif [[ ${KEYMAP} == main ]] ||
       [[ ${KEYMAP} == viins ]] ||
       [[ ${KEYMAP} = '' ]] ||
       [[ $1 = 'beam' ]]; then
    echo -ne '\e[5 q'
  fi
}
zle -N zle-keymap-select
zle-line-init() {
    zle -K viins # initiate `vi insert` as keymap (can be removed if `bindkey -V` has been set elsewhere)
    echo -ne "\e[5 q"
}
zle -N zle-line-init

echo -ne '\e[5 q' # Use beam shape cursor on startup.
preexec() { echo -ne '\e[5 q' ;} # Use beam shape cursor for each new prompt.

# Load zsh-syntax-highlighting; should be last.
source /usr/share/zsh/plugins/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh 2>/dev/null

[ -f ~/.fzf.zsh ] && source ~/.fzf.zsh
# Mimir git prompt
autoload -Uz add-zsh-hook
prompt_mimir_cmd() {
    if command -v mimir >/dev/null 2>&1; then
        mimir
    else
        echo  "[$PWD]"
    fi
}
add-zsh-hook precmd prompt_mimir_cmd

prompt_symbol='❯'
PROMPT="%(?.%F{magenta}.%F{red})${prompt_symbol}%f "

history() { fc -lim "*$@*" 1 }
[ -f $HOME/workspace/work_env.sh ] && source $HOME/workspace/work_env.sh

[ -f "$HOME/.secrets" ] && source "$HOME/.secrets"
[ -f "$HOME/.helpers" ] && source "$HOME/.helpers"
[ -f "$HOME/.aliasrc" ] && source "$HOME/.aliasrc"
