"""Contained regression tests. Usage: python test_security.py BINARY [BASELINE]."""
from pathlib import Path
import random
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BINARY = str(Path(sys.argv[1]).resolve())
BASELINE = str(Path(sys.argv[2]).resolve()) if len(sys.argv) > 2 else None
checks = 0


def run(args, binary=BINARY, stdin=None, timeout=5):
    return subprocess.run([binary, *map(str, args)], input=stdin,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          timeout=timeout, cwd=ROOT)


def check(result, expected, label):
    global checks
    assert result.returncode == expected, (label, result.returncode, result.stderr[:1000])
    checks += 1


with tempfile.TemporaryDirectory(prefix='pmars-security-', dir=ROOT / 'security') as folder:
    tmp = Path(folder)
    source = tmp / 'input.red'

    def warrior(body, expected=3, label='malformed'):
        source.write_bytes(b';redcode\n;assert 1\n' + body)
        check(run(['-r', '0', source]), expected, label)

    # Byte counters previously wrapped, even for legal whitespace and labels.
    warrior(b' ' * 512 + b'dat 0,0\n', 0, 'long whitespace')
    warrior(b'X' * 300 + b' dat 0,0\n', 0, 'long label')
    warrior(b'dat ' + b' ' * 512 + b'1,0\n', 0, 'long operand whitespace')
    warrior(b';name ' + b'N' * 7900 + b'\ndat 0,0\n', 0, 'long metadata')
    warrior(b';name ' + b'N' * 7900 + b'\n;author ' + b'A' * 7900 + b'\ndat 0,0\n', 0)
    warrior(b'dat 12345678901234567890,0\n', label='20 digit numeral')
    warrior(b'dat (' + b'\n', label='unclosed parenthesis')
    for expr in (b'1+', b'1&', b'1|', b'1!', b'1=', b'()', b'1/0', b'1%0',
                 b'2147483647+1', b'9223372036854775807+1',
                 b'(-2147483647-1)/-1', b'(-9223372036854775807-1)/-1',
                 b'999999999999999999999999999999999999'):
        # 32-bit-long arithmetic tests use a width-neutral overflow product.
        if expr in (b'2147483647+1', b'(-2147483647-1)/-1') and sys.platform != 'win32':
            continue
        warrior(b'dat ' + expr + b',0\n', label=repr(expr))
    warrior(b'dat ' + b'+' * 1000 + b'1,0\n', label='unary nesting')
    warrior(b'dat ' + b'(' * 1000 + b'1' + b')' * 1000 + b',0\n', label='parenthesis nesting')
    warrior(b'X equ X\ndat X,0\n', label='recursive equate')
    warrior(b'value equ ' + b'+'.join([b'1'] * 150) + b'\ndat value,0\n', 0, 'long flat equate')
    warrior((ROOT / 'security/fixtures/numb.red').read_bytes(), 0, 'Numb published warrior')
    numb = run(['-r', '0', ROOT / 'security/fixtures/numb.red'])
    expected = (ROOT / 'security/fixtures/numb.expected.txt').read_text()
    assert [line.rstrip() for line in numb.stdout.decode().splitlines()] == expected.splitlines(), 'Numb upstream assembly parity'
    checks += 1
    chain = b''.join(f'X{i} equ X{i+1}\n'.encode() for i in range(500))
    warrior(chain + b'X500 equ 1\ndat X0,0\n', label='deep equate chain')
    warrior(b'for 65536\nrof\ndat 0,0\n', label='FOR truncation')
    warrior(b'for 65535\nrof\ndat 0,0\n', 0, 'maximum empty FOR')
    warrior(b'for 65535\nfor 65535\nrof\nrof\ndat 0,0\n', label='macro work exhaustion')
    warrior(b'for 101\ndat 0,0\nrof\n', label='expanded instruction limit')
    warrior(b'for 0\n' + b'for 1\n' * 300 + b'dat 0,0\n' + b'rof\n' * 301 + b'dat 0,0\n', 0, 'skipped nesting')
    warrior(b';' + b'X' * 8000 + b'\ndat 0,0\n', label='long physical line')
    warrior(b'dat ' + b' ' * 7000 + b'\\\n' + b' ' * 1500 + b'0,0\n', label='long continuation')
    warrior(b'dat 0,0\x00\n', label='NUL byte')
    warrior(b';empty\n' * 65536 + b'dat 0,0\n', label='line number exhaustion')
    warrior(b';' + b'x' * 7900 + b'\n' * 1 + (b';' + b'x' * 7900 + b'\n') * 140,
            label='input byte exhaustion')
    warrior(b'LABEL' + b'X' * 7970 + b'\n', 0, 'large diagnostic')

    # Fixed bounds for argv/options, includes and stdin warrior counts.
    check(run(['999999999999999999999999-']), 2, 'stdin warrior count')
    check(run(['37-']), 2, 'stdin warrior array bound')
    check(run(['-r', '99999999999999999999999', source]), 2, 'numeric CLI overflow')
    check(run(['-F', '99999999999999999999999', source]), 2, 'fixed-position overflow')
    check(run(['-c', '9223372036854775807', source, source]), 2, 'cycle counter overflow')
    check(run(['-p', '2147483647', source, source, source]), 2, 'task queue counter overflow')
    options = tmp / 'bad.opt'
    options.write_text('"' + 'x' * 10000 + '"')
    check(run(['-@', options]), 2, 'oversized quoted option')
    options.write_text('x' * 10000)
    check(run(['-@', options]), 2, 'oversized option')
    options.write_text('-@ "' + str(options) + '"')
    check(run(['-@', options]), 2, 'cyclic option include')
    options.write_text('; ' + 'ignored' * 5000 + '\n-r 0\n')
    source.write_text(';redcode\n;assert 1\ndat 0,0\n')
    check(run(['-@', options, source]), 0, 'long option comment')
    source.write_text(';redcode\n;assert 1\n;debug\ndat 0,0\n')
    check(run(['-b', '-r', '1', source, source], stdin=b'\n'), 0, 'SERVER ignores submission debugger directives')
    source.write_text(';redcode\n;assert 1\n' + 'dat 999999,999999\n' * 1000)
    check(run(['-b', '-k', '-f', '-r', '1', '-s', '1000000', '-l', '1000', source, source]),
          0, 'warrior checksum wraps safely')
    other = tmp / 'other.red'
    source.write_text(';redcode\n;assert 1\npin 0\ndat 0,0\n')
    other.write_text(';redcode\n;assert 1\ndat 0,0\n')
    check(run(['-b', '-r', '0', '-Q', '2000', source, other]), 1, 'PIN zero cannot share private P-space')
    other.write_text(';redcode\n;assert 1\npin 0\ndat 0,0\n')
    check(run(['-b', '-r', '0', '-Q', '2000', source, other]), 2, 'explicit matching PINs share P-space')

    # Random bytes and truncated expressions: success or documented parse
    # failure is allowed; a signal, exception or timeout is never allowed.
    rng = random.Random(20261002)
    for i in range(200):
        body = bytes(rng.choice(b'0123456789()+-*/%!<>=&| ABCxyz\xff') for _ in range(rng.randrange(1, 300)))
        source.write_bytes(b';redcode\n;assert 1\ndat ' + body + b',0\n')
        result = run(['-r', '0', source])
        assert result.returncode in (0, 3), (i, result.returncode, result.stderr[:500])
        checks += 1

    # Real warriors: compare emitted load code and deterministic battles.
    warriors = sorted((ROOT / 'warriors').glob('*.red'))
    for path in warriors:
        args = ['-A', '-r', '0', path]
        result = run(args)
        check(result, 0, path.name)
        if BASELINE:
            before = run(args, BASELINE)
            assert before.returncode == 0 and result.stdout == before.stdout, path.name
            checks += 1
    for path in warriors:
        args = ['-b', '-k', '-r', '10', '-F', '1234', path, warriors[0]]
        result = run(args)
        check(result, 0, 'battle ' + path.name)
        if BASELINE:
            before = run(args, BASELINE)
            assert before.returncode == 0 and result.stdout == before.stdout, ('battle mismatch', path.name)
            checks += 1

print(f'PASS: {checks} security and compatibility checks')
