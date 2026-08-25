#!/usr/bin/env bash
set -u

DOTFILES="${DOTFILES:-}"
if [ -z "$DOTFILES" ]; then
	if [ -f /dotfiles/Makefile ]; then
		DOTFILES=/dotfiles
	else
		script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
		DOTFILES="$(dirname "$script_dir")"
	fi
fi

if [ ! -f "$DOTFILES/Makefile" ]; then
	echo "error: dotfiles repo not found (set DOTFILES=/path/to/repo)" >&2
	exit 2
fi
SKIP_PKG_CHECK="${SKIP_PKG_CHECK:-0}"

G='\033[32m' R='\033[31m' B='\033[1m' N='\033[0m'
pass=0 fail=0
pkgs=()
aur_pkgs=()
ok()   { pass=$((pass + 1)); printf "  ${G}PASS${N} %s\n" "$*"; }
bad()  { fail=$((fail + 1)); printf "  ${R}FAIL${N} %s\n" "$*"; }
head() { printf "\n${B}== %s ==${N}\n" "$*"; }

. /etc/os-release
echo "dotfiles test suite — ${PRETTY_NAME}"
uname -mr

head "shell syntax"
for f in .zshrc .zprofile; do
	if [ -f "$DOTFILES/$f" ]; then
		zsh -n "$DOTFILES/$f" && ok "zsh -n $f" || bad "zsh -n $f"
	fi
done
for f in .bashrc .bash_profile .profile .aliasrc .wprofile .winitrc toggle-git.sh; do
	if [ -f "$DOTFILES/$f" ]; then
		bash -n "$DOTFILES/$f" && ok "bash -n $f" || bad "bash -n $f"
	fi
done

head "packages.txt format"
valid_name='^[a-zA-Z0-9@._+:-]+$'
if [ ! -s "$DOTFILES/packages.txt" ]; then
	bad "packages.txt missing or empty"
else
	mapfile -t pkgs < <(grep -v '^[[:space:]]*$' "$DOTFILES/packages.txt")
	ok "packages.txt has ${#pkgs[@]} entries"
	dupes="$(printf '%s\n' "${pkgs[@]}" | sort | uniq -d)"
	if [ -z "$dupes" ]; then ok "no duplicate entries"; else bad "duplicates: $(echo $dupes | tr '\n' ' ')"; fi
	badnames=""
	for p in "${pkgs[@]}"; do [[ "$p" =~ $valid_name ]] || badnames="$badnames $p"; done
	if [ -z "$badnames" ]; then ok "all entries look like package names"; else bad "malformed:$badnames"; fi
fi

if [ "$SKIP_PKG_CHECK" != 1 ] && [ "${#pkgs[@]}" -gt 0 ]; then
	head "packages.txt repo availability"
	missing="$(printf '%s\n' "${pkgs[@]}" \
		| xargs -P 8 -I{} sh -c 'pacman -Si --quiet "{}" >/dev/null 2>&1 || pacman -Sg --quiet "{}" >/dev/null 2>&1 || echo "{}"' || true)"
	if [ -z "$missing" ]; then
		ok "all ${#pkgs[@]} packages/groups found in repos"
	else
		for m in $missing; do bad "not found in repos: $m"; done
	fi
fi

head "packages-aur.txt format"
if [ -s "$DOTFILES/packages-aur.txt" ]; then
	mapfile -t aur_pkgs < <(grep -v '^[[:space:]]*$' "$DOTFILES/packages-aur.txt")
	dupes="$(printf '%s\n' "${aur_pkgs[@]}" | sort | uniq -d)"
	badnames=""
	for p in "${aur_pkgs[@]}"; do [[ "$p" =~ $valid_name ]] || badnames="$badnames $p"; done
	if [ -z "$dupes" ] && [ -z "$badnames" ]; then
		ok "${#aur_pkgs[@]} AUR entries valid (repo availability needs paru, skipped)"
	else
		[ -n "$dupes" ] && bad "duplicates: $(echo $dupes | tr '\n' ' ')"
		[ -n "$badnames" ] && bad "malformed:$badnames"
	fi
fi

ignore_entries() {
	grep -v '^[[:space:]]*$' "$DOTFILES/.stow-local-ignore" 2>/dev/null | sed 's/\\//g'
}
is_ignored_rel() {
	[ "$(basename "$1")" = ".stow-local-ignore" ] && return 0
	ignore_entries | grep -Fxq "$1"
}

expected_items() {
	shopt -s nullglob
	local entry name
	for entry in "$DOTFILES"/* "$DOTFILES"/.[!.]*; do
		name="$(basename "$entry")"
		is_ignored_rel "$name" && continue
		printf '%s\n' "$name"
	done
}
mapfile -t expected < <(expected_items)

is_repo_link() {
	[ -L "$1" ] && [[ "$(readlink -f "$1")" == "$DOTFILES"* ]]
}

check_stowed_item() {
	local name="$1" target="$HOME/$1"
	if is_repo_link "$target"; then
		ok "$name -> $(readlink "$target")"
	elif [ -d "$target" ] && [ ! -L "$target" ]; then
		check_merged_dir "$name"
	else
		bad "$name is not a symlink into $DOTFILES"
	fi
}

check_merged_dir() {
	local rel="$1" child crel errors=0
	shopt -s nullglob
	for child in "$DOTFILES/$rel"/* "$DOTFILES/$rel"/.[!.]*; do
		crel="$rel/$(basename "$child")"
		is_ignored_rel "$crel" && continue
		if is_repo_link "$HOME/$crel"; then
			continue
		elif [ -d "$HOME/$crel" ] && [ ! -L "$HOME/$crel" ]; then
			check_merged_dir "$crel" || errors=$((errors + 1))
		else
			bad "$crel not linked into \$HOME"
			errors=$((errors + 1))
		fi
	done
	[ "$errors" -eq 0 ] && ok "$rel merged: children linked"
	return "$errors"
}

head "make stow (fresh sandboxed \$HOME)"
sandbox="$(mktemp -d)"
trap 'rm -rf "$sandbox"' EXIT
HOME="$sandbox"
export HOME
mkdir -p "$HOME"
log="$(mktemp)"
if (cd "$DOTFILES" && make stow) >"$log" 2>&1; then
	ok "make stow exited cleanly"
else
	bad "make stow failed:"
	tail -20 "$log" | sed 's/^/       /'
fi
for name in "${expected[@]}"; do
	check_stowed_item "$name"
done

head "make destow"
if (cd "$DOTFILES" && make destow) >"$log" 2>&1; then
	ok "make destow exited cleanly"
else
	bad "make destow failed:"
	tail -20 "$log" | sed 's/^/       /'
fi
leftovers=0
for name in "${expected[@]}"; do
	while IFS= read -r -d '' l; do
		bad "leftover link: ${l#"$HOME"/}"
		leftovers=$((leftovers + 1))
	done < <(find "$HOME/$name" -type l -print0 2>/dev/null)
done
[ "$leftovers" -eq 0 ] && ok "no leftover symlinks into repo"

rm -f "$log"

head "summary"
printf "  ${B}%d passed, %d failed${N}\n" "$pass" "$fail"
[ "$fail" -eq 0 ]
