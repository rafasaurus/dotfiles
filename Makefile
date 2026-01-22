stow_dirs = $(wildcard .)

.PHONY: stow restow destow install-prereqs install-paru install-paru-packages
.PHONY: install-udev install-gui install-themes install-android-env
.PHONY: install-film-android install-cursors
.PHONY: uninstall-gui uninstall-udev install-full uninstall-film-android

stow : check_dirs
	stow --target $(HOME) --verbose $(stow_dirs)

restow :
	stow --target $(HOME) --verbose --restow $(stow_dirs)

destow :
	stow -D --target $(HOME) --verbose $(stow_dirs)

install-prereqs :
	sudo pacman -S --needed - < packages.txt

install-cursors :
	rm -rf apple_cursor
	git clone https://github.com/ful1e5/apple_cursor
	cd apple_cursor && python3.8 -m venv env
	cd apple_cursor && source env/bin/activate && pip install clickgen && \
		git checkout v2.0.0 && \
		ctgen build.toml -s 30 -p x11 -d "bitmaps/macOS-BigSur" -n "dwlcursor" -c "Custom Sizes macOS XCursors" && \
		cp themes/dwlcursor ~/.icons/ -r

install-paru :
	[ -d paru ] || git clone https://aur.archlinux.org/paru.git 
	cd paru && makepkg -si
	rm -rf paru

install-paru-packages:
	paru -S --noconfirm ttf-apple-emoji startw qt5-styleplugins

install-udev :
	sudo cp -r etc/udev/rules.d/* /etc/udev/rules.d/
	sudo udevadm control --reload-rules && sudo udevadm trigger

reinstall : install-gui install-prereqs install-paru-packages

install-gui :
	cp .config/wall.png $(HOME)/.config/wall.png
	cd external/dwl && sudo make install -j
	cp patches/dwlb-config.h external/dwlb/config.h && cd external/dwlb && sudo make install -j
	cd external/wlbubble && sudo make install -j
	cd dwlb-status && make install

uninstall-gui :
	cd external/dwl && sudo make uninstall && make clean
	cd external/dwlb && sudo make uninstall -j && make clean
	cd external/wlbubble && sudo make uninstall -j && make clean
	cd dwlb-status && make uninstall && make clean
	sudo rm /usr/share/xsessions/dwm.desktop

uninstall-udev :
	sudo rm -r /etc/udev/rules.d/*

install-themes : 
	[ -d $(HOME)/.themes ] || mkdir $(HOME)/.themes
	git clone https://github.com/B00merang-Project/Mac-OS-9 ~/.themes/Mac-OS-9

install-mimir :
	cd ./external/mimir && git chckout main && make install

install-full :  install-paru install-prereqs install-mimir
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
	# if mimeapps exists as file delele it, if its a symlink or does not exists do nothing
	[ -L ~/.config/mimeapps.list ] || ([ -f ~/.config/mimeapps.list ] && rm ~/.config/mimeapps.list ) || echo ""
	[ -d $(HOME)/.config ] || mkdir $(HOME)/.config
	[ -d $(HOME)/.local ] || mkdir $(HOME)/.local
	[ -d $(HOME)/.local/share ] || mkdir -p $(HOME)/.local/share
	[ -d $(HOME)/.local/bin ] || mkdir -p $(HOME)/.local/bin
	[ -d $(HOME)/.local/share/applications ] || mkdir -p $(HOME)/.local/share/applications
	[ -d $(HOME)/.local/share/fonts ] || mkdir -p $(HOME)/.local/share/fonts
	[ -d $(HOME)/.cache/zsh ] || mkdir -p $(HOME)/.cache/zsh
	[ -d $(HOME)/.todo ] || mkdir $(HOME)/.todo

