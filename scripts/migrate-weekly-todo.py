#!/usr/bin/env python3
"""Remove the retired weekly script, including Syncthing replacement files."""
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
HOME = Path(os.environ['HOME'])
legacy_directory = HOME / 'obsidian'
legacy_script = legacy_directory / 'weekly_todo_roller.py'


def points_to(link, target):
    # Compare lexically so dangling links work, without following other links.
    return link.is_symlink() and os.path.abspath(
        link.parent / os.readlink(link)) == str(target)


def main():
    if legacy_directory.is_symlink():
        # Stow may have folded the entire old package directory into one link.
        if points_to(legacy_directory, ROOT / 'common/obsidian') and not legacy_directory.exists():
            legacy_directory.unlink()
            print('Removed dangling legacy Obsidian directory symlink')
            return 0
        print('Legacy ~/obsidian is a directory symlink; review it manually before migration.', file=sys.stderr)
        return 1
    if os.path.lexists(legacy_script):
        legacy_script.unlink()
        print('Removed retired ~/obsidian/weekly_todo_roller.py')
    return 0


if __name__ == '__main__':
    sys.exit(main())
