#
# Install manually
# libxft-bgra-git
# ttf-devicons
# ttf-dejavu for monospaced
# If we want to use universal style for both qt and gtk, we shall
# install qt5-styleplugins package from git or AUR

stow_dirs = $(wildcard .)
TMUX_VERSION=2.8
IGNORE_FLAGS= --ignore "Makefile" \
		--ignore ".docs" \
		--ignore "\.gitignore" \
		--ignore "\.gitmodules" \
		--ignore "\.git-prompt.sh" \
		--ignore ".suckless.d" \
		--ignore "slstatus" \
		--ignore "README" \
		--ignore "utils" \
		--ignore "wallpaper" \
		--ignore "etc" \
		--ignore "tmux-${TMUX_VERSION}" \
		# --ignore ".gtkrc-2.0" \

.PHONY : stow
stow :
	# if mimeapps exists as file delele it, if its a symlink or does not exists do nothing
	[ -L ~/.config/mimeapps.list ] || ([ -f ~/.config/mimeapps.list ] && rm ~/.config/mimeapps.list ) || echo ""
	[ -L ~/.gtkrc-2.0 ] || ([ -d ~/.gtkrc-2.0 ] && rm ~/.gtkrc-2.0) || echo ""
	[[ -d $(HOME)/.config ]] || mkdir $(HOME)/.config
	[[ -d $(HOME)/.local ]] || mkdir $(HOME)/.local
	[[ -d $(HOME)/.local/share ]] || mkdir -p $(HOME)/.local/share
	[[ -d $(HOME)/.local/bin ]] || mkdir -p $(HOME)/.local/bin
	[[ -d $(HOME)/.local/share/applications ]] || mkdir -p $(HOME)/.local/share/applications
	[[ -d $(HOME)/.local/share/fonts ]] || mkdir -p $(HOME)/.local/share/fonts
	[[ -d $(HOME)/.cache/zsh ]] || mkdir -p $(HOME)/.cache/zsh
	[[ -d $(HOME)/.todo ]] || mkdir $(HOME)/.todo
	stow --target $(HOME) --verbose $(stow_dirs) $(IGNORE_FLAGS)
	@echo ''
	@echo ''
	@echo '******************************************************'
	@echo 'Please read what should be manually installed'
	@echo 'Primarily udevs and fonts'
	@echo '******************************************************'
	@echo ''

.PHONY : restow
restow :
	stow --target $(HOME) --verbose --restow $(stow_dirs) $(IGNORE_FLAGS)

.PHONY : destow
destow :
	stow -D --target $(HOME) --verbose $(stow_dirs) $(IGNORE_FLAGS)

.PHONY : install-prereqs
install-prereqs :
	sudo pacman -S stow \
		base-devel neovim git rofi sxhkd arandr ranger dunst sxiv imagemagick ffmpeg \
		networkmanager ttf-joypixels ttf-linux-libertine ttf-inconsolata \
		xwallpaper python-pip pipewire pipewire-pulse wireplumber alsa-utils \
		maim unrar unzip youtube-dl zathura zathura-djvu zathura-pdf-mupdf \
		poppler highlight fzf acpilight xorg-xprop xorg-xinit xorg-xwininfo xorg-server \
		openssh ttf-liberation ttf-dejavu ttf-fira-code fontconfig ttf-roboto ttf-font-awesome \
		unclutter zsh xclip wget curl libevent ncurses libnotify pamixer upower the_silver_searcher \
		redshift bluez-cups bluez-utils bluez pass qtpass mlocate lsof lxappearance xcursor-themes \
		cmake libc++abi libc++ cronie

	sudo pip install pywal undervolt wpm
	@echo ''
	@echo ''
	@echo '******************************************************'
	@echo 'Please read what should be manually installed'
	@echo 'Install Fira Code from aur'
	@echo 'enable pipewire-pulse.service and socket per user'
	@echo 'systemctl --user enable pipewire-pulse.service'
	@echo 'systemctl --user enable pipewire-pulse.socket'
	@echo 'in order to use xsessions, you should run <install-gui>'
	@echo '******************************************************'
	@echo ''

.PHONY : install-paru
install-paru :
	[ -d paru ] || git clone https://aur.archlinux.org/paru.git 
	cd paru && makepkg -si
.PHONY : install-paru-packages
install-paru-packages :
	paru compton-tryone-git
.PHONY : install-prereqs-paru
install-prereqs-paru :
	paru todo.sh compton-tryone
.PHONY : install-udev
install-udev :
	sudo cp -r etc/udev/rules.d/* /etc/udev/rules.d/
	sudo udevadm control --reload-rules && sudo udevadm trigger
.PHONY : install-tmux
install-tmux :
	wget https://github.com/tmux/tmux/releases/download/${TMUX_VERSION}/tmux-${TMUX_VERSION}.tar.gz
	tar xf tmux-${TMUX_VERSION}.tar.gz
	rm -f tmux-${TMUX_VERSION}.tar.gz
	cd tmux-${TMUX_VERSION} && ./configure && make && sudo make install
.PHONY : install-gui
install-gui :
	cd .suckless.d/dwm && sudo make install -j
	cd .suckless.d/dwmblocks && sudo make install -j
	cd .suckless.d/slstatus && sudo make install -j
	cd .suckless.d/dmenu && sudo make install -j
	cd .suckless.d/slock && sudo make install -j
	sudo cp dwm.desktop /usr/share/xsessions
.PHONY : clean-gui
clean-gui :
	cd .suckless.d/dwm && sudo make clean
	cd .suckless.d/dwmblocks && sudo make clean
	cd .suckless.d/slstatus && sudo make clean
	cd .suckless.d/dmenu && sudo make clean
	cd .suckless.d/slock && sudo make clean
.PHONY : uninstall-gui
uninstall-gui :
	cd .suckless.d/dwm && sudo make uninstall
	cd .suckless.d/dwmblocks && sudo make uninstall
	cd .suckless.d/slstatus && sudo make uninstall
	cd .suckless.d/dmenu && sudo make uninstall
	cd .suckless.d/slock && sudo make uninstall
	sudo rm /usr/share/xsessions/dwm.desktop

.PHONY : uninstall-udev
uninstall-udev :
	sudo rm -r /etc/udev/rules.d/*
.PHONY : install-full
install-full :  install-paru install-prereqs
	echo "done" .PHONY : install-android-env
install-android-env :
	cp .local/bin/mimir_armv7l $(shell dirname `which sh`)
.PHONY : install-film-android
install-film-android :
	mkdir -p $(HOME)/.local/bin
	cp -r .local/bin/luts/ $(HOME)/.local/bin/ && echo "copied luts"
	cp -r .local/bin/film $(shell dirname `which sh`) && echo "copied film script"
.PHONY : uninstall-film-android
.PHONY : uninstall-film-android
uninstall-film-android :
	rm -r $(shell dirname `which sh`)/512x512 $(shell which film)
