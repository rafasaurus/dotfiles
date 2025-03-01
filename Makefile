#
# Install manually
# libxft-bgra-git
# ttf-devicons
# ttf-dejavu for monospaced
# If we want to use universal style for both qt and gtk, we shall
# install qt5-styleplugins package from git or AUR

stow_dirs = $(wildcard .)
TMUX_VERSION=3.4
IGNORE_FLAGS= --ignore "Makefile" \
		--ignore ".config/wall.png" \
		--ignore ".docs" \
		--ignore "\.gitignore" \
		--ignore "\.gitmodules" \
		--ignore "external" \
		--ignore "README" \
		--ignore "etc" \
		--ignore "tmux-${TMUX_VERSION}" \
		--ignore "paru" \
		--ignore ".gtkrc-2.0" \
		--ignore "patches" \

# Phony targets for make
.PHONY: stow restow destow install-prereqs install-paru
.PHONY: install-udev install-tmux install-gui lean-gui
.PHONY: uninstall-gui uninstall-udev install-full install-android-env
.PHONY: install-film-android uninstall-film-android

stow :
	# if mimeapps exists as file delele it, if its a symlink or does not exists do nothing
	[ -L ~/.config/mimeapps.list ] || ([ -f ~/.config/mimeapps.list ] && rm ~/.config/mimeapps.list ) || echo ""
	[ -L ~/.gtkrc-2.0 ] || ([ -d ~/.gtkrc-2.0 ] && rm ~/.gtkrc-2.0) || echo ""
	[ -d $(HOME)/.config ] || mkdir $(HOME)/.config
	[ -d $(HOME)/.local ] || mkdir $(HOME)/.local
	[ -d $(HOME)/.local/share ] || mkdir -p $(HOME)/.local/share
	[ -d $(HOME)/.local/bin ] || mkdir -p $(HOME)/.local/bin
	[ -d $(HOME)/.local/share/applications ] || mkdir -p $(HOME)/.local/share/applications
	[ -d $(HOME)/.local/share/fonts ] || mkdir -p $(HOME)/.local/share/fonts
	[ -d $(HOME)/.cache/zsh ] || mkdir -p $(HOME)/.cache/zsh
	[ -d $(HOME)/.todo ] || mkdir $(HOME)/.todo
	stow --target $(HOME) --verbose $(stow_dirs) $(IGNORE_FLAGS)
	@echo ''
	@echo ''
	@echo '******************************************************'
	@echo 'Please read what should be manually installed'
	@echo 'Primarily udevs and fonts'
	@echo '******************************************************'
	@echo ''

restow :
	stow --target $(HOME) --verbose --restow $(stow_dirs) $(IGNORE_FLAGS)

destow :
	stow -D --target $(HOME) --verbose $(stow_dirs) $(IGNORE_FLAGS)

install-prereqs :
	sudo pacman -S --needed - < packages.txt

	@echo ''
	@echo ''
	@echo '******************************************************'
	@echo 'Please read what should be manually installed'
	@echo 'enable pipewire-pulse.service and socket per user'
	@echo 'systemctl --user enable pipewire-pulse.service'
	@echo 'systemctl --user enable pipewire-pulse.socket'
	@echo 'in order to use xsessions, you should run <install-gui>'
	@echo '******************************************************'
	@echo ''

install-paru :
	[ -d paru ] || git clone https://aur.archlinux.org/paru.git 
	cd paru && makepkg -si
	paru -S --noconfirm ttf-twemoji-color
install-udev :
	sudo cp -r etc/udev/rules.d/* /etc/udev/rules.d/
	sudo udevadm control --reload-rules && sudo udevadm trigger
install-tmux :
	wget https://github.com/tmux/tmux/releases/download/${TMUX_VERSION}/tmux-${TMUX_VERSION}.tar.gz
	tar xf tmux-${TMUX_VERSION}.tar.gz
	rm -f tmux-${TMUX_VERSION}.tar.gz
	cd tmux-${TMUX_VERSION} && ./configure && make && sudo make install
install-gui :
	cd external/dwm && sudo make install -j
	cd external/dwmblocks-async && sudo make install -j
	cd external/dmenu && sudo make install -j
	cd external/slock && sudo make install -j
	sudo cp dwm.desktop /usr/share/xsessions
uninstall-gui :
	cd external/dwm && sudo make uninstall
	cd external/dwmblocks-async && sudo make uninstall
	cd external/dmenu && sudo make uninstall
	cd external/slock && sudo make uninstall
	sudo rm /usr/share/xsessions/dwm.desktop

uninstall-udev :
	sudo rm -r /etc/udev/rules.d/*
install-mimir:
	cd ./external/mimir && make install
install-full :  install-paru install-prereqs install-mimir
	echo "done full installation"
install-android-env :
	cp .local/bin/mimir_armv7l $(shell dirname `which sh`)
install-film-android :
	mkdir -p $(HOME)/.local/bin
	cp -r .local/bin/luts/ $(HOME)/.local/bin/ && echo "copied luts"
	cp -r .local/bin/film $(shell dirname `which sh`) && echo "copied film script"
uninstall-film-android :
	rm -r $(shell dirname `which sh`)/512x512 $(shell which film)
