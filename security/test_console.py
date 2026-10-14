"""Verify console/server debugger behavior. Usage: test_console.py CONSOLE SERVER."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CONSOLE, SERVER = (str(Path(arg).resolve()) for arg in sys.argv[1:])


def run(binary, args, commands=b'quit\n'):
    return subprocess.run([binary, *map(str, args)], input=commands,
                          capture_output=True, timeout=5, cwd=ROOT)


checks = 0
with tempfile.TemporaryDirectory(dir=ROOT / 'security') as folder:
    source = Path(folder) / 'debug.red'
    for directive in ('', ';debug', ';trace', ';break'):
        source.write_text(f';redcode\n;assert 1\n{directive}\ndat 0,0\n')
        args = ['-b', '-r', '1', source, source]
        # Explicit -e and embedded directives must still enter console cdb.
        console = run(CONSOLE, args if directive else ['-e', *args])
        assert console.returncode == 4, (directive, console)
        # The same input must not enter the server debugger, even with -e.
        server = run(SERVER, args if directive else ['-e', *args])
        assert server.returncode == 0, (directive, server)
        checks += 2
    source.write_text(';redcode\n;assert 1\ndat 0,0\n')
    for binary, variant in ((CONSOLE, 1), (SERVER, 0)):
        result = run(binary, ['-b', '-r', '0', '-Q', '1001', source])
        assert result.returncode == variant, result
        checks += 1
print(f'PASS: {checks} console/server debugger checks')
