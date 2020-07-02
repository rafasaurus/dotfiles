stow_dirs = $(wildcard .)
IGNORE_FLAGS= --ignore "Makefile" \
		--ignore ".docs" \
		--ignore ".fonts" \
		--ignore "\.gitignore" \
		--ignore "\.gitmodules" \
		--ignore "\.git-prompt.sh" \
		--ignore "dwm" \
		--ignore "slstatus" \
		--ignore "README" \
		--ignore "utils" \
		--ignore "wallpaper" \
		--ignore ".gtkrc-2.0" \
		--ignore ".local/services"

.PHONY : stow
stow :
	stow --target $(HOME) --verbose $(stow_dirs) $(IGNORE_FLAGS)
	# sudo ln -s $(PWD)/.local/services/slock.service /etc/systemd/system/slock.service
	# sudo systemctl enable slock.service

.PHONY : restow
restow:
	stow --target $(HOME) --verbose --restow $(stow_dirs) $(IGNORE_FLAGS)

.PHONY : delete
delete :
	stow -D --target $(HOME) --verbose $(stow_dirs) $(IGNORE_FLAGS)
	# sudo rm /etc/systemd/system/slock.service
	# sudo systemctl disable slock.service

install-prereqs :
	sudo pacman -S stow \
					vim nvim \
					git \
					rofi \
					sxhkd \
					arandr \
					ranger \
					dunst \
					sxiv imagemagick \
					ffmpeg \
					networkmanager \
					ttf-joypixels ttf-symbola \
					pulseaudio pulseaudio-alsa alsa-utils \
					maim \
					unrar unzip \
					youtube-dl \
					zathura zathura-djvu \
					poppler \
					highlight \
					fzf \
					xorg-xbacklight xorg-xprop xorg-xinit xorg-xwininfo xorg-server \


