"""Exercise the native GDI renderer and patched simulator on Windows.

Usage: python security/test_gdi.py src/pmars-gdi.exe [baseline.exe]
Redirected stdin suppresses the interactive end-of-battle key wait.
"""
from pathlib import Path
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BINARY = str(Path(sys.argv[1]).resolve())
BASELINE = str(Path(sys.argv[2]).resolve()) if len(sys.argv) > 2 else None
ENV = dict(os.environ)

subprocess.run([sys.executable, str(ROOT / "security/test_security.py"),
                BINARY, *([BASELINE] if BASELINE else [])],
               cwd=ROOT, env=ENV, input=b"", check=True)

checks = 0
def run(args, commands=b"", binary=BINARY, expected=0):
    result = subprocess.run([binary, *args], input=commands, cwd=ROOT,
                            env=ENV, capture_output=True, timeout=15)
    assert result.returncode == expected, (args, result.returncode, result.stderr)
    return result

# Core display cell sizes and read/write/execute/decrement/increment levels.
for mode in (0, 1, 2, 9):
    for level in (0, 1, 3, 4):
        args = ["-b", "-k", "-r", "2", "-c", "1000", "-F", "1234",
                "-v", str(800 + mode * 10 + level),
                "warriors/validate.red", "warriors/sunset.red"]
        result = run(args)
        if BASELINE:
            before = run(args[:args.index("-v")] + args[args.index("-v") + 2:],
                         binary=BASELINE)
            assert result.stdout == before.stdout, (mode, level, "battle mismatch")
        checks += 1

# Split/close panels, change display configuration during debugging, print
# registers/results, step the simulator, and leave the debugger cleanly.
run(["-b", "-e", "-v", "804", "-c", "100", "warriors/validate.red"],
    b"lis 0,3\nswi 2\ncls\nclo\ndis 814\ndis clear\nreg\npro\nstep\ncon\n")
checks += 1
run(["-b", "-e", "-v", "804", "warriors/sunset.red"], b"quit\n", expected=4)
checks += 1
for mode in ("320x240", "1024x768", "640x480@16", "640x480:noframe",
             "640x480:-resizable", "640x480:fullscreen"):
    run(["-b", "-m", mode, "-v", "804", "-c", "100", "warriors/validate.red"])
    checks += 1
for mode in ("0x480", "65536x480", "999999999999999999999x480", "640x-1",
             "640x480junk", "640x480@33"):
    run(["-b", "-m", mode, "-c", "1", "warriors/validate.red"], expected=1)
    checks += 1
print(f"PASS: {checks} GDI mode and debugger checks")
