# Configuration
export PATH=/usr/lib/ccache:$PATH
PS1="\[$(tput bold)\]\[$(tput setaf 1)\][\[$(tput setaf 3)\]\u\[$(tput setaf 2)\]@\[$(tput setaf 4)\]\h \[$(tput setaf 5)\]\W\[$(tput setaf 1)\]]\[$(tput setaf 7)\]\\$ \[$(tput sgr0)\]"
# export PATH=$PATH:$HOME/github/config/scripts

# Aliases
alias performance="echo \"pstate-frequency --set g powersave\" && sudo pstate-frequency --set -g performance"
alias powersave="echo \"pstate-frequency --set g powersave\" && sudo pstate-frequency --set -g powersave"
alias xm="xmodmap.sh"
alias rxm="resetXmodmap.sh"
alias w3m="/usr/bin/w3m"
alias sl="ls"
alias r="ranger"
alias i="sudo apt-get install"
alias upd="sudo apt update -y"
alias upg="sudo apt upgrade -y"
alias open="xdg-open"
alias red="redshift -O"
alias ured="redshift -x"
alias jlinkexe='/opt/SEGGER/JLink/JLinkExe'
# alias weather='curl -4 http://wttr.in/Seattle'
alias weather='curl wttr.in/Yerevan'
alias vim="nvim"
alias ll='ls -alF'
alias la='ls -A'
alias l='ls -CF'
alias opeth="open ~/Pictures/opeth/2019-03-09_23-26-16.png &"
alias xo="xdg-open"
alias cp='rsync -aP'
alias cal='cal -A 2'
alias youtube-dl-mp3='youtube-dl --extract-audio --audio-format mp3'
alias youtube-dl-playlist='youtube-dl -i -f mp4 --yes-playlist'
alias=CC=clang
alias=CXX=/usr/bin/clang++

# Variables
source ~/git-prompt.sh
export EDITOR="vim"
export NDK="/home/rafael/Android/Sdk/ndk/20.0.5594570/"
export HOST_TAG="linux-x86_64"
export PATH=$PATH:$HOME/workspace/ardupilot/Tools/autotest
export ROS_PACKAGE_PATH=${ROS_PACKAGE_PATH}:$HOME/github/ORB_SLAM/
export HISTFILE=~/.bash_eternal_history
export HISTTIMEFORMAT="[%F %T] "
export PYTHONPATH="${PYTHONPATH}:/opt/movidius/caffe/python"
# unified bash history across tmux, and other terminal emulators
export PROMPT_COMMAND="history -a;$PROMPT_COMMAND"
export PYTHONPATH=$PYTHONPATH:/home/rafael/workspace/nn/models/research:/home/rafael/workspace/nn/models/research/slim
export PATH=$PATH:$HOME/workspace/khach/scripts
export PATH=$PATH:$HOME/workspace/khach/build/tools
export PATH=$PATH:$HOME/workspace/khach/build/tools
export PATH=$PATH:$HOME/workspace/khach/build/khach
export PATH=$PATH:/opt/telegram
export PATH=$PATH:$HOME/.scripts
export TERM=xterm-256color

# Extract archive
function extract {
    if [ -z "$1" ]; then
        echo "Usage: extract <path/file_name>.<zip|rar|bz2|gz|tar|tbz2|tgz|Z|7z|xz|ex|tar.bz2|tar.gz|tar.xz>"
    else
        if [ -f $1 ] ; then
            case $1 in
                *.tar.bz2)   tar xvjf ./$1    ;;
                .tar.gz)    tar xvzf ./$1    ;;
                *.tar.xz)    tar xvJf ./$1    ;;
                *.lzma)      unlzma ./$1      ;;
                *.bz2)       bunzip2 ./$1     ;;
                *.rar)       unrar x -ad ./$1 ;;
                *.gz)        gunzip ./$1      ;;
                *.tar)       tar xvf ./$1     ;;
                *.tbz2)      tar xvjf ./$1    ;;
                *.tgz)       tar xvzf ./$1    ;;
                *.zip)       unzip ./$1       ;;
                *.Z)         uncompress ./$1  ;;
                *.7z)        7z x ./$1        ;;
                *.xz)        unxz ./$1        ;;
                *.exe)       cabextract ./$1  ;;
                *)           echo "extract: '$1' - unknown archive method" ;;
            esac
        else
            echo "$1 - file does not exist"
        fi
    fi
}

stty -ixon
[ -f ~/.fzf.bash ] && source ~/.fzf.bash
