"""Regression coverage for behavior introduced in the 0.9.6 candidate.
Usage: python security/test_096.py binary [upstream-binary]
"""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
binary = str(Path(sys.argv[1]).resolve())
baseline = str(Path(sys.argv[2]).resolve()) if len(sys.argv) > 2 else None
checks = 0
with tempfile.TemporaryDirectory(dir=ROOT / 'security') as tmp:
    path = Path(tmp) / 'macro.red'
    for body, expected in [
        ('block equ dat 1,2\n equ dat 3,4\nblock\n', 0),
        ('block equ dat 1,2\n;assert 1\n equ dat 3,4\nblock\n', 0),
        ('block equ dat 1,2\n;assert 0\n equ dat 3,4\nblock\n', 3),
        ('val equ 2\n;assert val == 2\ndat val,0\n', 0),
        (';assert (a=3)==3 && a>=3 && a<=3 && a!=4\ndat a,0\n', 0),
        ('dat 1<2,2>1\ndat 2<=2,2>=2\n', 0),
        (' ' * 512 + 'dat 1,2\n', 0),
    ]:
        path.write_text(';redcode\n;assert 1\n' + body)
        def run(exe):
            return subprocess.run([exe, '-A', '-r', '0', str(path)],
                                  input=b'', capture_output=True, timeout=5, cwd=ROOT)
        result = run(binary)
        assert result.returncode == expected, (body[:200], result.stderr)
        # Upstream still hangs on token offsets past 255; never feed it this case.
        if baseline and len(body) < 256:
            before = run(baseline)
            assert before.returncode == expected, (body[:200], before.stderr)
            if expected == 0:
                assert result.stdout == before.stdout, (body[:200], result.stdout, before.stdout)
        checks += 1
print(f'PASS: {checks} 0.9.6 assembler regression checks')
