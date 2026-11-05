# Weekly todo script and Syncthing

Run `weekly-todo-roller` from your shell. GNU Stow manages the executable at
`~/.local/bin/weekly-todo-roller`; its source is
`common/.local/bin/weekly-todo-roller`. Its existing behavior and note location
(`~/obsidian/weekly-todos`) are unchanged.

Keep executable code outside the Syncthing-synced notes directory. Sync the
Markdown notes, not Stow-managed scripts or their symlinks. This avoids a
regular synced file conflicting with the symlink Stow expects to manage.

## Updating another machine

1. Compare any `~/obsidian/weekly_todo_roller.py` replacement with the Git
   version and preserve local edits outside the synced directory.
2. Move that old script out of the synced directory, coordinating removal
   across synced machines. Do not delete a modified copy without reviewing it.
3. Pull this change, then run `make stow` or `make restow`. Both automatically
   remove the exact old Stow script symlink, including a dangling link left by
   pulling before destowing. A dangling folded `~/obsidian` link pointing to
   this checkout's removed `common/obsidian` directory is also removed safely.
   Regular files and unexpected links are preserved and require manual review.
   No `destow` before pulling is necessary. Run `make doctor` afterward if available.
4. Update shortcuts from `python ~/obsidian/weekly_todo_roller.py` to
   `weekly-todo-roller` (or `~/.local/bin/weekly-todo-roller`).

There is no compatibility symlink inside `~/obsidian`, deliberately. This
change does not recreate an Obsidian vault or configure Syncthing.
