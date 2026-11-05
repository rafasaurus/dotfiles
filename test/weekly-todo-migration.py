#!/usr/bin/env python3
"""Isolated legacy-link migration tests; never touch the actual HOME."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class MigrationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.repo = self.base / 'repo'
        self.home = self.base / 'home'
        (self.repo / 'scripts').mkdir(parents=True)
        self.home.mkdir()
        self.script = self.repo / 'scripts/migrate-weekly-todo.py'
        shutil.copyfile(ROOT / 'scripts/migrate-weekly-todo.py', self.script)
        self.old = self.home / 'obsidian/weekly_todo_roller.py'

    def run_migration(self):
        return subprocess.run(['python3', str(self.script)],
                              env=dict(os.environ, HOME=str(self.home)),
                              capture_output=True, text=True)

    def test_absent_is_noop(self):
        self.assertEqual(self.run_migration().returncode, 0)
        self.assertFalse(self.old.parent.exists())

    def test_todo_calls_relocated_roller_without_legacy_script(self):
        binary = self.home / '.local/bin/weekly-todo-roller'
        binary.parent.mkdir(parents=True)
        binary.write_text(
            'import os\nfrom pathlib import Path\n'
            'Path(os.environ["HOME"], "roller-called").write_text("yes")\n')
        result = subprocess.run(['bash', str(ROOT / 'common/.local/bin/todo')],
                                env=dict(os.environ, HOME=str(self.home), EDITOR='/bin/true'),
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual((self.home / 'roller-called').read_text(), 'yes')
        self.assertFalse(os.path.lexists(self.old))

    def test_dangling_link_removed_and_notes_preserved(self):
        self.old.parent.mkdir()
        note = self.old.parent / 'note.md'
        note.write_text('keep me')
        self.old.symlink_to(os.path.relpath(
            self.repo / 'common/obsidian/weekly_todo_roller.py', self.old.parent))
        self.assertEqual(self.run_migration().returncode, 0)
        self.assertFalse(os.path.lexists(self.old))
        self.assertEqual(note.read_text(), 'keep me')
        self.assertEqual(self.run_migration().returncode, 0)

    def test_syncthing_replacement_removed_and_notes_preserved(self):
        self.old.parent.mkdir()
        self.old.write_text('synced script')
        note = self.old.parent / 'note.md'
        note.write_text('keep me')
        self.assertEqual(self.run_migration().returncode, 0)
        self.assertFalse(os.path.lexists(self.old))
        self.assertEqual(note.read_text(), 'keep me')

    def test_other_script_link_removed_without_removing_target(self):
        self.old.parent.mkdir()
        target = self.home / 'other-script'
        target.write_text('keep target')
        self.old.symlink_to(target)
        self.assertEqual(self.run_migration().returncode, 0)
        self.assertFalse(os.path.lexists(self.old))
        self.assertEqual(target.read_text(), 'keep target')

    def test_dangling_folded_directory_removed(self):
        self.old.parent.symlink_to(self.repo / 'common/obsidian')
        self.assertEqual(self.run_migration().returncode, 0)
        self.assertFalse(os.path.lexists(self.old.parent))

    def test_live_directory_symlink_not_traversed(self):
        target = self.repo / 'common/obsidian'
        target.mkdir(parents=True)
        (target / self.old.name).write_text('preserve')
        self.old.parent.symlink_to(target)
        self.assertEqual(self.run_migration().returncode, 1)
        self.assertEqual(self.old.read_text(), 'preserve')


if __name__ == '__main__':
    unittest.main()
