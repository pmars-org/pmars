"""Installed config lookup from unrelated working directories and PATH."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
checks = 0
for argument in sys.argv[1:]:
    binary = Path(argument).resolve()
    with tempfile.TemporaryDirectory(prefix='config-path-', dir=ROOT / 'security') as temp:
        root = Path(temp)
        bindir = root / 'package with spaces' / 'bin'
        config = bindir.parent / 'config'
        share = bindir.parent / 'share' / 'pmars'
        cwd = root / 'unrelated'
        home = root / 'override'
        for directory in (bindir, config, share, cwd, home):
            directory.mkdir(parents=True, exist_ok=True)
        executable = bindir / binary.name
        shutil.copy2(binary, executable)
        env = dict(os.environ, PATH=str(bindir) + os.pathsep + os.environ.get('PATH', ''))
        env.pop('PMARSHOME', None)
        warrior = cwd / 'test.red'
        warrior.write_text(';redcode\n;assert CORESIZE == 800\ndat 0,0\n')
        valid = '-s 800 -l 20 -d 20 -r 0\n'

        def launch(args, **kwargs):
            # Windows CreateProcess searches the parent's PATH, ignoring the
            # replacement environment for executable lookup. A shell resolves
            # the bare command using the test PATH, as an end user would.
            if os.name == 'nt' and args[0] == binary.name:
                args = [os.environ.get('COMSPEC', 'cmd.exe'), '/d', '/c', *args]
            return subprocess.run(args, cwd=cwd, capture_output=True, timeout=5, **kwargs)

        def run(name='lookup.opt', expected=0, command=None, extra_env=None):
            global checks
            result = launch([command or binary.name, '-@', name, str(warrior)],
                            env=extra_env or env, input=b'')
            assert result.returncode == expected, (binary.name, name, result)
            checks += 1

        # Each supported layout must work alone, with PATH launch and spaces.
        for directory in (config, share, bindir):
            option = directory / 'lookup.opt'
            option.write_text(valid)
            run()
            run(command=str(executable))
            option.unlink()
        # Search precedence: local > PMARSHOME > config > share > executable.
        ordered = (cwd, home, config, share, bindir)
        env['PMARSHOME'] = str(home)
        for directory in ordered:
            (directory / 'lookup.opt').write_text('-not-an-option\n')
        for directory in ordered:
            option = directory / 'lookup.opt'
            option.write_text(valid)
            run()
            option.unlink()
        env['PMARSHOME'] = ''
        (config / 'lookup.opt').write_text(valid)
        run()  # Empty PMARSHOME must be harmless.
        run('./lookup.opt', expected=2)  # Explicit missing path never falls back.
        run(str(cwd / 'lookup.opt'), expected=2)
        run('absent.opt', expected=2)
        run(str(config / 'lookup.opt'))
        (config / 'nested.opt').write_text('-@ lookup.opt\n')
        run('nested.opt')
        (config / 'cycle.opt').write_text('-@ cycle.opt\n')
        run('cycle.opt', expected=2)
        if os.name != 'nt':
            link = cwd / 'linked-pmars'
            link.symlink_to(executable)
            run(command=str(link))
        if 'server' not in binary.stem:
            (config / 'pmars.mac').write_text('probe=echo CONFIG_LOOKUP_OK\n')
            result = launch([binary.name, '-e', '-s', '800', '-l', '20', '-d', '20', str(warrior)],
                            env=env, input=b'm probe\nquit\n')
            assert result.returncode == 4 and b'CONFIG_LOOKUP_OK' in result.stdout + result.stderr, result
            checks += 1
print(f'PASS: {checks} installed configuration lookup checks')
