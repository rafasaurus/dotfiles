#
# ~/.bash_profile
#

# [[ -f ~/.bashrc ]] && . ~/.bashrc

export QT_STYLE_OVERRIDE="gtk2"
export QT_QPA_PLATFORMTHEME="gtk2"
# export QT_QPA_PLATFORMTHEME="qt5ct"
# export QT_STYLE_OVERRIDE="Fusion"
export QT_SCALE_FACTOR=1.25
export GTK2_RC_FILES="$HOME/.gtkrc-2.0"
export _JAVA_AWT_WM_NONREPARENTING=1 # Fixing misbehaving Java applications
export THEME=dark
export XDG_SESSION_TYPE=X11
export XDG_SESSION_CLASS=user
export XDG_MENU_PREFIX=lxde-
export XDG_SESSION_DESKTOP="dwm"
export XDG_CURRENT_DESKTOP="dwm"
export XDG_CONFIG_HOME="$HOME/.config"
export XDG_DATA_DIRS="/usr/local/share:/usr/share:/var/lib/flatpak/exports/share:$HOME/.local/share/flatpak/exports/share"
export XDG_CONFIG_DIRS="/usr/share/upstart/xdg:/etc/xdg"
export XDG_CACHE_HOME="$HOME/.cache"
export XDG_DATA_HOME="$HOME/.local/share/"
export XDG_RUNTIME_DIR="$HOME/.cache/xdgr/"
export DESKTOP_SESSION="dwm"
export SUDO_ASKPASS="$HOME/.local/bin/dmenupass"
export BROWSER=chromium
export VISUAL="/usr/bin/vim" # for crontab -e
export EDITOR=nvim 
export TERMINAL=alacritty
export BROWSER=firefox
export LD_LIBRARY_PATH="/usr/local/lib"
export LIBVA_DRIVER_NAME=iHD
export GTK_USE_PORTAL=0

# This is the list for lf icons:
export LF_ICONS="di=📁:\
fi=📃:\
tw=🤝:\
ow=📂:\
ln=⛓:\
or=❌:\
ex=🎯:\
*.txt=✍:\
*.mom=✍:\
*.me=✍:\
*.ms=✍:\
*.png=🖼:\
*.webp=🖼:\
*.ico=🖼:\
*.jpg=📸:\
*.jpe=📸:\
*.jpeg=📸:\
*.gif=🖼:\
*.svg=🗺:\
*.tif=🖼:\
*.tiff=🖼:\
*.xcf=🖌:\
*.html=🌎:\
*.xml=📰:\
*.gpg=🔒:\
*.css=🎨:\
*.pdf=📚:\
*.djvu=📚:\
*.epub=📚:\
*.csv=📓:\
*.xlsx=📓:\
*.tex=📜:\
*.md=📘:\
*.r=📊:\
*.R=📊:\
*.rmd=📊:\
*.Rmd=📊:\
*.m=📊:\
*.mp3=🎵:\
*.opus=🎵:\
*.ogg=🎵:\
*.m4a=🎵:\
*.flac=🎼:\
*.wav=🎼:\
*.mkv=🎥:\
*.mp4=🎥:\
*.webm=🎥:\
*.mpeg=🎥:\
*.avi=🎥:\
*.mov=🎥:\
*.mpg=🎥:\
*.wmv=🎥:\
*.m4b=🎥:\
*.flv=🎥:\
*.zip=📦:\
*.rar=📦:\
*.7z=📦:\
*.tar.gz=📦:\
*.z64=🎮:\
*.v64=🎮:\
*.n64=🎮:\
*.gba=🎮:\
*.nes=🎮:\
*.gdi=🎮:\
*.1=ℹ:\
*.nfo=ℹ:\
*.info=ℹ:\
*.log=📙:\
*.iso=📀:\
*.img=📀:\
*.bib=🎓:\
*.ged=👪:\
*.part=💔:\
*.torrent=🔽:\
*.jar=♨:\
*.java=♨:\
"
# [[ -f ~/.profile ]] && . ~/.profile
