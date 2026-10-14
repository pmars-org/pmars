"""Run SDL3 regressions, event integration and (Windows) static import audit.
Usage: python security/test_sdl3.py build-mingw [--native]
--native additionally opens native desktop windows briefly.
"""
from pathlib import Path
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BUILD = Path(sys.argv[1]).resolve()
EXT = '.exe' if os.name == 'nt' else ''
BINARY = BUILD / ('pmars-sdl3' + EXT)
EVENTS = BUILD / ('pmars-sdl3-events' + EXT)
SERVER = BUILD / ('pmars-server' + EXT)

def run(args, *, env=None, expected=0, cwd=ROOT):
    p = subprocess.run([str(x) for x in args], input=b'', capture_output=True,
                       cwd=cwd, env=env, timeout=90)
    if p.returncode != expected:
        raise AssertionError((args, p.returncode, p.stdout, p.stderr))
    if p.stdout:
        print(p.stdout.decode(errors='replace').strip())
    return p

run([sys.executable, ROOT/'security/test_sdl.py', BINARY, SERVER])
run([sys.executable, ROOT/'security/test_096.py', BINARY])
args = ['-b', '-v', '834', '-c', '100', 'warriors/validate.red', 'warriors/sunset.red']
for driver in ['dummy'] + (['windows' if os.name == 'nt' else 'cocoa' if sys.platform == 'darwin' else 'x11'] if '--native' in sys.argv else []):
    env = dict(os.environ, SDL_VIDEO_DRIVER=driver)
    env.pop('PMARS_SDL3_EXIT_TEST', None)
    run([EVENTS, *args], env=env)
    for mode, status in [('close', 4), ('ctrl-c', 4), ('escape', 4), ('endwait', 0)]:
        run([EVENTS, *args], env=dict(env, PMARS_SDL3_EXIT_TEST=mode), expected=status)
    (ROOT / 'sdl3-events.bmp').replace(BUILD / f'sdl3-events-{driver}.bmp')
    print(f'PASS: {driver} close, Ctrl+C, Escape, final key wait')

cache = (BUILD / 'CMakeCache.txt').read_text()
static = re.search(r'^PMARS_STATIC:BOOL=(ON|TRUE|1)$', cache, re.M) is not None
if os.name == 'nt':
    # objdump must come from the same MinGW-w64 toolchain used for the build.
    pe = subprocess.check_output(['objdump', '-p', str(BINARY)], text=True)
    assert 'pei-x86-64' in pe and re.search(r'Subsystem\s+00000003', pe), 'not x64 console PE'
    imports = re.findall(r'DLL Name:\s*(\S+)', pe)
    if not static:
        assert any(x.lower() == 'sdl3.dll' for x in imports), imports
        print('PASS: x64 console PE; shared SDL3 import')

if os.name == 'nt' and static:
    system = {'advapi32.dll', 'gdi32.dll', 'imm32.dll', 'kernel32.dll', 'ole32.dll',
              'oleaut32.dll', 'setupapi.dll', 'shell32.dll', 'user32.dll',
              'version.dll', 'winmm.dll', 'msvcrt.dll', 'ucrtbase.dll'}
    assert imports and all(x.lower() in system or x.lower().startswith('api-ms-win-crt-')
                           for x in imports), imports
    print('PASS: x64 console PE; Windows system imports only: ' + ', '.join(imports))
    # Launch from an otherwise empty directory with no compiler/SDL search path.
    with tempfile.TemporaryDirectory(prefix='sdl3-clean-', dir=BUILD) as tmp:
        exe = Path(tmp)/BINARY.name
        shutil.copy2(BINARY, exe)
        env = dict(os.environ, PATH=str(Path(os.environ['SystemRoot'])/'System32'),
                   SDL_VIDEO_DRIVER='windows' if '--native' in sys.argv else 'dummy')
        result = run([exe, '-b', '-v', '804', '-r', '1', '-c', '100',
                      ROOT/'warriors/validate.red', ROOT/'warriors/sunset.red'],
                     env=env, cwd=tmp)
        assert result.stdout and not (Path(tmp)/'stdout.txt').exists()
        bad = run([exe, '-A', ROOT/'does-not-exist.red'], env=env, cwd=tmp, expected=3)
        assert bad.stderr, 'stderr lost'
    print('PASS: clean-directory launch, console stdout/stderr and exit status without SDL/compiler runtime DLLs')
