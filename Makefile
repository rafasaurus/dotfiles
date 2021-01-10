#
# Install manually
# libxft-bgra-git
# ttf-devicons
#

stow_dirs = $(wildcard .)
IGNORE_FLAGS= --ignore "Makefile" \
		--ignore ".docs" \
		--ignore ".fonts" \
		--ignore "\.gitignore" \
		--ignore "\.gitmodules" \
		--ignore "\.git-prompt.sh" \
		--ignore ".suckless.d" \
		--ignore "slstatus" \
		--ignore "README" \
		--ignore "utils" \
		--ignore "wallpaper" \
		--ignore "todo"
		# --ignore ".gtkrc-2.0" \

.PHONY : stow
stow :
	[[ -d $(HOME)/.config ]] || mkdir $(HOME)/.config # making .local directory
	[[ -d $(HOME)/.local ]] || mkdir $(HOME)/.local # making .local directory
	[[ -d $(HOME)/.local/share ]] || mkdir -p $(HOME)/.local/share # making .local/share directory
	[[ -d $(HOME)/.local/bin ]] || mkdir -p $(HOME)/.local/bin # making .local/share directory
	[[ -d $(HOME)/.local/share/applications ]] || mkdir -p $(HOME)/.local/share/applications # making .local/share directory
	[[ -d $(HOME)/.cache/zsh ]] || mkdir -p $(HOME)/.cache/zsh # making .local/share directory
	stow --target $(HOME) --verbose $(stow_dirs) $(IGNORE_FLAGS)
	@echo ''
	@echo ''
	@echo '******************************************************'
	@echo 'Please read what should be manually installed'
	@echo '******************************************************'
	@echo ''

.PHONY : restow
restow :
	stow --target $(HOME) --verbose --restow $(stow_dirs) $(IGNORE_FLAGS)

.PHONY : delete
delete :
	stow -D --target $(HOME) --verbose $(stow_dirs) $(IGNORE_FLAGS)

.PHONY : install-prereqs
install-prereqs :
	sudo pacman -S stow \
					vim neovim \
					git \
					rofi \
					sxhkd \
					arandr \
					ranger \
					dunst \
					sxiv imagemagick \
					ffmpeg \
					networkmanager \
					ttf-joypixels \
					ttf-linux-libertine \
					ttf-inconsolata \
					ttf-font-awesome \
					xwallpaper \
					python-pip \
					pulseaudio pulseaudio-alsa alsa-utils \
					maim \
					unrar unzip \
					youtube-dl \
					zathura zathura-djvu \
					zathura-pdf-poppler \
					poppler \
					highlight \
					fzf \
					xorg-xbacklight xorg-xprop xorg-xinit xorg-xwininfo xorg-server\
					openssh \
					ttf-liberation
	sudo pip install pywal undervolt
	@echo ''
	@echo ''
	@echo '******************************************************'
	@echo 'Please read what should be manually installed'
	@echo '******************************************************'
	@echo ''


