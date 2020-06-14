stow_dirs = $(wildcard .)
IGNORE_FLAGS= --ignore "Makefile" \
		--ignore ".docs" \
		--ignore ".fonts" \
		--ignore "\.gitignore" \
		--ignore "\.git-prompt.sh" \
		--ignore "README" \
		--ignore "utils" \
		--ignore "wallpaper" \
		--ignore ".gtkrc-2.0"

.PHONY : stow
stow :
	stow --target $(HOME) --verbose $(stow_dirs) $(IGNORE_FLAGS)

.PHONY : restow
restow:
	stow --target $(HOME) --verbose --restow $(stow_dirs) $(IGNORE_FLAGS)

.PHONY : delete
delete :
	stow -D --target $(HOME) --verbose $(stow_dirs) $(IGNORE_FLAGS)
clean :
	rm -rf ~/.local ~/.config
