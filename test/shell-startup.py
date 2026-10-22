#!/usr/bin/env python3
"""Check completion cache compilation without touching the user's cache."""

import os
from pathlib import Path
import pty
import subprocess
import tempfile


rc = Path(__file__).resolve().parents[1] / "common" / ".zshrc"
with tempfile.TemporaryDirectory(prefix="shell-startup-") as directory:
    home = Path(directory)
    (home / ".cache" / "zsh").mkdir(parents=True)
    master, slave = pty.openpty()
    try:
        def startup():
            result = subprocess.run(
                [
                    "zsh", "-fic",
                    'zmodload zsh/zprof; source "$1"; '
                    '(( $+functions[compdef] && $+_comps[git] )) || exit 1; '
                    '[[ $_comp_options == *globdots* ]] || exit 1; zprof',
                    "shell-startup", str(rc),
                ],
                env={**os.environ, "HOME": directory, "ZDOTDIR": directory},
                stdin=slave, capture_output=True, text=True, check=True,
            )
            assert "compaudit" in result.stdout, "security audit must still run"

        startup()
        dump = home / ".zcompdump"
        compiled = home / ".zcompdump.zwc"
        assert dump.is_file() and compiled.is_file()
        initial = compiled.stat().st_mtime_ns
        startup()
        assert compiled.stat().st_mtime_ns == initial, "unchanged cache was recompiled"
        os.utime(compiled, (1, 1))
        startup()
        assert compiled.stat().st_mtime_ns > 1_000_000_000, "stale cache not rebuilt"
    finally:
        os.close(master)
        os.close(slave)

print("PASS: completions, security audit, cache reuse and stale-cache rebuild")
