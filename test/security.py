#!/usr/bin/env python3
"""Isolated security regressions; no root, desktop session or network required."""
import importlib.machinery
import importlib.util
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.dont_write_bytecode = True


def executable(path, body):
    path.write_text('#!/bin/sh\n' + body)
    path.chmod(0o700)


class SecurityTests(unittest.TestCase):
    def test_status_escaping(self):
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            executable(temp / 'curl', 'printf "%s" "$WEATHER_TEXT"\n')
            (temp / 'probe.c').write_text(
                '#define main collector_main\n'
                f'#include "{ROOT / "dwlb-status/dwlb-status.c"}"\n'
                '#undef main\n#include <assert.h>\n'
                'int main(void) {\n'
                ' char small[8], weather[128];\n'
                ' escape_status_text(small, 2, "^"); assert(!strcmp(small, ""));\n'
                ' escape_status_text(small, 3, "^"); assert(!strcmp(small, "^^"));\n'
                ' escape_status_text(small, 4, "ab^"); assert(!strcmp(small, "ab"));\n'
                ' escape_status_text(small, sizeof small, "a\\nb\\033");\n'
                ' assert(!strcmp(small, "ab"));\n'
                ' escape_status_text(NULL, 0, "ignored");\n'
                ' weather_text(weather, sizeof weather); puts(weather); return 0; }\n'
            )
            subprocess.run(['cc', '-O2', str(temp / 'probe.c'),
                            '-o', str(temp / 'probe')], check=True)
            env = dict(os.environ, PATH=directory + ':' + os.environ['PATH'])
            for raw, expected in [
                ('^lm(printf marker)CLICK^lm()', '^^lm(printf marker)CLICK^^lm()'),
                ('☀️  +20C', '🔆 +20C'),
                ('Sunny +20C', 'Sunny +20C'),
                ('\x1b^us(printf marker)scroll^us()', '^^us(printf marker)scroll^^us()'),
                ('a' * 126 + '^', 'a' * 126),
            ]:
                output = subprocess.check_output([str(temp / 'probe')],
                    env=dict(env, WEATHER_TEXT=raw), text=True).rstrip('\n')
                self.assertEqual(output, expected)

    def test_qr_cleanup_and_exclusive_creation(self):
        loader = importlib.machinery.SourceFileLoader(
            'qr_clipboard', str(ROOT / 'common/.local/bin/helpers/qr-clipboard'))
        spec = importlib.util.spec_from_loader(loader.name, loader)
        qr = importlib.util.module_from_spec(spec)
        loader.exec_module(qr)
        real_run = subprocess.run
        old_handlers = {sig: signal.getsignal(sig) for sig in
                        (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)}
        try:
            with tempfile.TemporaryDirectory() as directory:
                image = Path(directory) / 'qr.png'
                for failure in (None, 'wl-paste', 'qrencode', 'nsxiv', 'signal', 'replacement'):
                    shredded = []

                    def run(argv, **kwargs):
                        if argv[0] == failure:
                            raise subprocess.CalledProcessError(1, argv)
                        if argv[0] == 'wl-paste':
                            return subprocess.CompletedProcess(argv, 0, stdout=b'clipboard secret')
                        if argv[0] == 'qrencode':
                            os.write(kwargs['stdout'], b'fake QR image')
                        elif argv[0] == 'nsxiv':
                            self.assertEqual(image.read_bytes(), b'fake QR image')
                            if failure == 'signal':
                                qr.interrupted(signal.SIGTERM, None)
                            if failure == 'replacement':
                                image.unlink()
                                image.write_bytes(b'new unrelated file')
                        elif argv[0] == 'shred':
                            self.assertTrue(image.exists(), 'shred must precede deletion')
                            shredded.append(True)
                            return real_run(argv, **kwargs)
                        return subprocess.CompletedProcess(argv, 0)

                    with patch.object(qr, 'Path', return_value=image), \
                         patch.object(qr.subprocess, 'run', side_effect=run):
                        if failure == 'signal':
                            with self.assertRaises(SystemExit):
                                qr.main()
                        elif failure and failure != 'replacement':
                            with self.assertRaises(subprocess.CalledProcessError):
                                qr.main()
                        else:
                            qr.main()
                    self.assertTrue(shredded)
                    if failure == 'replacement':
                        self.assertEqual(image.read_bytes(), b'new unrelated file')
                        image.unlink()
                    else:
                        self.assertFalse(image.exists())

                victim = Path(directory) / 'victim'
                victim.write_bytes(b'do not modify')
                for symlink in (False, True):
                    if symlink:
                        image.symlink_to(victim)
                    else:
                        image.write_bytes(b'existing QR')
                    with patch.object(qr, 'Path', return_value=image):
                        with self.assertRaises(FileExistsError):
                            qr.main()
                    self.assertEqual(victim.read_bytes(), b'do not modify')
                    self.assertTrue(image.exists())
                    image.unlink()
        finally:
            for sig, handler in old_handlers.items():
                signal.signal(sig, handler)

    def test_udev_uninstall_is_scoped(self):
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            commands = temp / 'bin'
            commands.mkdir()
            executable(commands / 'sudo', 'exec "$@"\n')
            executable(commands / 'udevadm', 'exit 0\n')
            rules = temp / 'rules'
            rules.mkdir()
            for source in (ROOT / 'etc/udev/rules.d').glob('*.rules'):
                (rules / source.name).write_text(source.read_text())
                self.assertNotIn('0666', source.read_text())
            unrelated = rules / '10-unrelated.rules'
            unrelated.write_text('keep me')
            subprocess.run(['make', '--silent', 'uninstall-udev', f'UDEV_RULES_DIR={rules}'],
                cwd=ROOT, check=True,
                env=dict(os.environ, PATH=str(commands) + ':' + os.environ['PATH']))
            self.assertEqual(list(rules.iterdir()), [unrelated])
            self.assertEqual(unrelated.read_text(), 'keep me')

    def test_bootstrap_rejects_wrong_checksum(self):
        with tempfile.TemporaryDirectory() as directory:
            temp = Path(directory)
            executable(temp / 'curl',
                       'while [ "$1" != "-o" ]; do shift; done\n'
                       'printf "untrusted code" > "$2"\n')
            vim_plug = temp / '.vim/autoload/plug.vim'
            vim_plug.parent.mkdir(parents=True)
            vim_plug.write_text('existing trusted code')
            env = dict(os.environ, HOME=directory, XDG_DATA_HOME=str(temp / 'data'),
                       PATH=directory + ':' + os.environ['PATH'])
            result = subprocess.run(['sh', str(ROOT / 'common/.local/bin/install-vim-plug')],
                                    env=env, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('checksum mismatch', result.stderr)
            self.assertEqual(vim_plug.read_text(), 'existing trusted code')
            self.assertFalse((temp / 'data/nvim/site/autoload/plug.vim').exists())


if __name__ == '__main__':
    unittest.main()
