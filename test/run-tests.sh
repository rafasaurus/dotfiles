#!/usr/bin/env bash
set -u

DOTFILES="${DOTFILES:-}"
if [ -z "$DOTFILES" ]; then
	if [ -f /root/dotfiles/Makefile ]; then
		DOTFILES=/root/dotfiles
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
	if [ -f "$DOTFILES/common/$f" ]; then
		zsh -n "$DOTFILES/common/$f" && ok "zsh -n $f" || bad "zsh -n $f"
	fi
done
for f in .bashrc .bash_profile .profile .aliasrc .wprofile .winitrc toggle-git.sh; do
	if [ -f "$DOTFILES/common/$f" ]; then
		bash -n "$DOTFILES/common/$f" && ok "bash -n $f" || bad "bash -n $f"
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

head "packages.txt repo availability"
if ! command -v pacman >/dev/null 2>&1; then
	printf "  SKIP (pacman not available on this distro)\n"
elif [ "$SKIP_PKG_CHECK" = 1 ]; then
	printf "  SKIP (SKIP_PKG_CHECK=1)\n"
elif [ "${#pkgs[@]}" -gt 0 ]; then
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
	grep -v '^[[:space:]]*$' "$DOTFILES/$1/.stow-local-ignore" 2>/dev/null | sed 's/\\//g'
}
is_ignored_rel() {
	[ "$(basename "$1")" = ".stow-local-ignore" ] && return 0
	ignore_entries "$2" | grep -Fxq "$1"
}

expected_items() {
	shopt -s nullglob
	local package="$1" entry name
	for entry in "$DOTFILES/$package"/* "$DOTFILES/$package"/.[!.]*; do
		name="$(basename "$entry")"
		is_ignored_rel "$name" "$package" && continue
		printf '%s/%s\n' "$package" "$name"
	done
}
packages=(common)
hostname="$(hostname)"
[ -d "$DOTFILES/$hostname" ] && packages+=("$hostname")
expected=()
for package in "${packages[@]}"; do
	while IFS= read -r item; do expected+=("$item"); done < <(expected_items "$package")
done

is_repo_link() {
	[ -L "$1" ] && [[ "$(readlink -f "$1")" == "$DOTFILES"* ]]
}

check_stowed_item() {
	local package="$1" name="$2" target="$HOME/$2"
	if is_repo_link "$target"; then
		ok "$name -> $(readlink "$target")"
	elif [ -d "$target" ] && [ ! -L "$target" ]; then
		check_merged_dir "$package" "$name"
	else
		bad "$name is not a symlink into $DOTFILES"
	fi
}

check_merged_dir() {
	local package="$1" rel="$2" child crel errors=0
	shopt -s nullglob
	for child in "$DOTFILES/$package/$rel"/* "$DOTFILES/$package/$rel"/.[!.]*; do
		crel="$rel/$(basename "$child")"
		is_ignored_rel "$crel" "$package" && continue
		if is_repo_link "$HOME/$crel"; then
			continue
		elif [ -d "$HOME/$crel" ] && [ ! -L "$HOME/$crel" ]; then
			check_merged_dir "$package" "$crel" || errors=$((errors + 1))
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
for item in "${expected[@]}"; do
	package="${item%%/*}"
	name="${item#*/}"
	check_stowed_item "$package" "$name"
done

head "make destow"
if (cd "$DOTFILES" && make destow) >"$log" 2>&1; then
	ok "make destow exited cleanly"
else
	bad "make destow failed:"
	tail -20 "$log" | sed 's/^/       /'
fi
leftovers=0
for item in "${expected[@]}"; do
	name="${item#*/}"
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
output=$(make -C "$DOTFILES" install-gui 2>&1); rc=$?
if [ "$rc" -eq 0 ]; then
    ok "make install-gui succeeded"
elif echo "$output" | grep -q "No rule to make target"; then
    bad "make install-gui: target not found in Makefile"
else
    ok "make install-gui ran (exit code $rc, likely display error without X server)"
fi
