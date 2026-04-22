stow_dirs = $(wildcard .)

.DEFAULT_GOAL := stow

.PHONY: all stow restow destow install-prereqs install-paru \
	install-aur install-udev install-gui install-themes \
	install-mimir install-full install-android-env install-film-android \
	install-neovim reinstall reinstall-gui \
	uninstall-gui uninstall-udev check_dirs install-cursors toggle help \

help:
	@printf "%-28s %s\n" "Target" "Description"
	@printf "%-28s %s\n" "------" "-----------"
	@printf "%-28s %s\n" "stow"              "Symlink dotfiles into HOME (default)"
	@printf "%-28s %s\n" "restow"            "Re-symlink dotfiles (refresh)"
	@printf "%-28s %s\n" "destow"            "Remove dotfile symlinks from HOME"
	@printf "%-28s %s\n" "install-full"      "Bootstrap: paru + pacman pkgs + AUR pkgs + mimir"
	@printf "%-28s %s\n" "install-prereqs"   "Install official packages via pacman (packages.txt)"
	@printf "%-28s %s\n" "install-paru"      "Install paru AUR helper (if not present)"
	@printf "%-28s %s\n" "install-aur"       "Install AUR packages via paru (packages-aur.txt)"
	@printf "%-28s %s\n" "install-gui"       "Build and install dwl, dwlb, dwlb-status, wlbubble"
	@printf "%-28s %s\n" "reinstall"         "Reinstall GUI + packages"
	@printf "%-28s %s\n" "reinstall-gui"     "Reinstall GUI only"
	@printf "%-28s %s\n" "uninstall-gui"     "Uninstall dwl and dwlb-status"
	@printf "%-28s %s\n" "install-udev"      "Deploy udev rules and reload"
	@printf "%-28s %s\n" "uninstall-udev"    "Remove all udev rules"
	@printf "%-28s %s\n" "install-themes"    "Clone Mac-OS-9 GTK theme"
	@printf "%-28s %s\n" "install-cursors"   "Build and install dwlcursor (Apple cursor)"
	@printf "%-28s %s\n" "install-mimir"     "Install mimir from source"
	@printf "%-28s %s\n" "install-neovim"    "Install neovim nightly AppImage"
	@printf "%-28s %s\n" "install-android-env" "Install mimir ARM binary for Android"
	@printf "%-28s %s\n" "install-film-android" "Install film tools for Android device"
	@printf "%-28s %s\n" "toggle"            "Toggle git remote visibility"

toggle:
	./toggle-git.sh

stow : check_dirs
	stow --target $(HOME) --verbose $(stow_dirs)

restow :
	stow --target $(HOME) --verbose --restow $(stow_dirs)

destow :
	stow -D --target $(HOME) --verbose $(stow_dirs)

install-prereqs :
	sudo pacman -S --needed - < packages.txt

install-paru :
	@if ! command -v paru >/dev/null; then \
		git clone https://aur.archlinux.org/paru.git && \
		cd paru && makepkg -si --noconfirm && \
		cd .. && rm -rf paru; \
	fi

install-aur :
	paru -S --needed - < packages-aur.txt

install-udev :
	sudo cp -r etc/udev/rules.d/* /etc/udev/rules.d/
	sudo udevadm control --reload-rules && sudo udevadm trigger

reinstall : uninstall-gui install-gui install-prereqs install-aur
reinstall-gui: uninstall-gui install-gui

install-gui :
	cp .config/wall.png $(HOME)/.config/wall.png
	sudo $(MAKE) -C external/dwl install -j
	sudo $(MAKE) -C external/wlbubble install -j
	$(MAKE) -C dwlb-status install
	cd external/dwlb && \
		cp ../../patches/dwlb-config.h config.h && \
		sed -i 's/CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-format-truncation -g/CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-format-truncation -O2 -march=native/' Makefile && \
		sudo $(MAKE) install -j && \
		git checkout -f .

uninstall-gui :
	sudo $(MAKE) -C external/dwl uninstall
	$(MAKE) -C dwlb-status uninstall

uninstall-udev :
	sudo rm -r /etc/udev/rules.d/*

install-themes :
	[ -d $(HOME)/.themes/Mac-OS-9 ] || \
		git clone https://github.com/B00merang-Project/Mac-OS-9 ~/.themes/Mac-OS-9

install-mimir :
	cd ./external/mimir && git checkout main && make install

install-full : install-paru install-prereqs install-aur install-mimir
	echo "done full installation"

install-android-env :
	cp .local/bin/mimir_armv7l $(shell dirname `which sh`)

install-film-android :
	apt install exiftool which
	mkdir -p $(HOME)/.local/bin
	cp -r .local/bin/luts/ $(HOME)/.local/bin/ && echo "copied luts"
	cp -r .local/bin/film $(shell dirname `which sh`) && echo "copied film script"

uninstall-film-android :
	rm -r $(shell dirname `which sh`)/512x512 $(shell which film)

check_dirs:
	[ ! -f ~/.config/mimeapps.list ] || [ -L ~/.config/mimeapps.list ] || rm ~/.config/mimeapps.list
	[ -d $(HOME)/.config ] || mkdir $(HOME)/.config
	[ -d $(HOME)/.local ] || mkdir $(HOME)/.local
	[ -d $(HOME)/.local/share ] || mkdir -p $(HOME)/.local/share
	[ -d $(HOME)/.local/bin ] || mkdir -p $(HOME)/.local/bin
	[ -d $(HOME)/.local/share/applications ] || mkdir -p $(HOME)/.local/share/applications
	[ -d $(HOME)/.local/share/fonts ] || mkdir -p $(HOME)/.local/share/fonts
	[ -d $(HOME)/.cache/zsh ] || mkdir -p $(HOME)/.cache/zsh
	[ -d $(HOME)/.todo ] || mkdir $(HOME)/.todo

install-cursors:
	rm -rf apple_cursor
	git clone https://github.com/ful1e5/apple_cursor
	cd apple_cursor && \
	python -m venv env && \
	. env/bin/activate && \
	pip install clickgen && \
	git checkout v2.0.0 && \
	wget https://github.com/ful1e5/apple_cursor/releases/download/v1.2.0/bitmaps.zip && \
	unzip bitmaps.zip && \
	mv macOSBigSur/* bitmaps/macOS-BigSur/ && \
	ctgen build.toml -s 31 -p x11 -d "bitmaps/macOS-BigSur" -n "dwlcursor" -c "Custom Sizes macOS XCursors" && \
	cp themes/dwlcursor ~/.icons/ -r
	@echo "you have to install cursor with nwg-look in wayland"

install-neovim:
	@set -e; \
	BASE_URL="https://github.com/neovim/neovim/releases/download/nightly"; \
	INSTALL_DIR="$(HOME)/.local/bin"; \
	APPIMAGE="nvim-linux-x86_64.appimage"; \
	[ -d "$$INSTALL_DIR" ] || { echo "$$INSTALL_DIR does not exist"; exit 1; }; \
	case "$$(uname -m)" in \
		aarch64|arm64) APPIMAGE="nvim-linux-arm64.appimage" ;; \
	esac; \
	curl -L "$$BASE_URL/$$APPIMAGE" -o "$$INSTALL_DIR/nvim"; \
	chmod +x "$$INSTALL_DIR/nvim"
