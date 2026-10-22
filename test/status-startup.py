#!/usr/bin/env python3
"""Regression: slow/offline HTTP must not block the bar or volume signals."""
import os
from pathlib import Path
import select
import signal
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def scenario(offline):
    with tempfile.TemporaryDirectory(prefix='status-startup-') as directory:
        temp = Path(directory)
        volume = temp / 'volume'
        volume.write_text('42')
        helpers = {
            'curl': 'sleep 2\n' + ('exit 1\n' if offline else
                'case "$*" in\n*goldprice*) printf \'{"price":"3110.34768"}\' ;;\n'
                '*) printf "Sunny +20C" ;;\nesac\n'),
            'pamixer': f'case "$1" in --get-mute) echo false ;; *) cat "{volume}" ;; esac\n',
            'pactl': 'exit 0\n', 'airpods': 'exit 0\n',
            'bluetoothctl': 'echo "Powered: yes"\n',
            'switch-theme': 'exit 0\n', 'nmcli': 'exit 0\n',
        }
        for name, body in helpers.items():
            path = temp / name
            path.write_text('#!/bin/sh\n' + body)
            path.chmod(0o755)
        env = dict(os.environ, PATH=directory + ':' + os.environ['PATH'])
        start = time.monotonic()
        process = subprocess.Popen([str(ROOT / 'dwlb-status/dwlb-status')], env=env,
                                   stdout=subprocess.PIPE, start_new_session=True)
        pending = b''

        def line_with(fragment, timeout):
            nonlocal pending
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                while b'\n' in pending:
                    line, pending = pending.split(b'\n', 1)
                    if fragment in line:
                        return line
                if select.select([process.stdout], [], [], max(0, deadline - time.monotonic()))[0]:
                    chunk = os.read(process.stdout.fileno(), 65536)
                    assert chunk, 'Collector exited early'
                    pending += chunk
            raise AssertionError(f'Timed out waiting for {fragment!r}')

        try:
            line_with(b'42%', 1)
            first = time.monotonic() - start
            assert first < 1, first
            volume.write_text('43')
            os.kill(process.pid, signal.SIGRTMIN)
            line_with(b'43%', .7)
            if not offline:
                line_with(b'Sunny +20C', 4)
                # Both workers started together; gold must also arrive promptly.
                # It may have arrived in an earlier status frame.
                line_with(b'$100.00/g', 2)
            else:
                time.sleep(2.2)
                assert process.poll() is None
            print(f'PASS {"offline" if offline else "slow HTTP"}: first frame {first:.3f}s, volume refresh remains responsive')
        finally:
            os.killpg(process.pid, signal.SIGTERM)
            process.wait(timeout=5)


if __name__ == '__main__':
    scenario(False)
    scenario(True)
