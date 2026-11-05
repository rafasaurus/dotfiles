# Weekly todo script and Syncthing

Run `weekly-todo-roller` from your shell. GNU Stow manages the executable at
`~/.local/bin/weekly-todo-roller`; its source is
`common/.local/bin/weekly-todo-roller`. Its existing behavior and note location
(`~/obsidian/weekly-todos`) are unchanged.

Keep executable code outside the Syncthing-synced notes directory. Sync the
Markdown notes, not Stow-managed scripts or their symlinks. This avoids a
regular synced file conflicting with the symlink Stow expects to manage.

## Updating another machine

1. Pull this change, then run `make stow` or `make restow`. Both automatically
   remove `~/obsidian/weekly_todo_roller.py`, whether it is a symlink or a
   regular file replaced by Syncthing. Rafael confirmed these are identical
   copies across machines; the canonical script now lives in `.local/bin`.
   A dangling folded `~/obsidian` link pointing to
   this checkout's removed `common/obsidian` directory is also removed safely.
   No `destow` before pulling is necessary. Run `make doctor` afterward if available.
2. Update shortcuts from `python ~/obsidian/weekly_todo_roller.py` to
   `weekly-todo-roller` (or `~/.local/bin/weekly-todo-roller`).

There is no compatibility symlink inside `~/obsidian`, deliberately. This
change does not recreate an Obsidian vault or configure Syncthing.
