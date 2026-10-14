"""Smoke-test the actual release archive, including an isolated installed run."""
import argparse
import json
import os
from pathlib import Path, PurePosixPath
import stat
import subprocess
import tempfile
import zipfile

from package_release import ROOT, PLATFORMS, archive_name, programs, version


def test_archive(path, target, ver):
    prefix = f'pmars-ai-{ver}/'
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
        assert len(names) == len(archive.infolist()), 'Duplicate archive entries'
        assert archive.testzip() is None, 'Corrupt archive'
        for name in names:
            assert name.startswith(prefix) and '..' not in PurePosixPath(name).parts, name
            assert '/.git/' not in name and not name.endswith(('.o', '.dll')), name
        for name in ('src/Makefile', 'src/configpath.c', 'CMakeLists.txt', 'COPYING',
                     'config/nano.opt', 'config/pmars.mac', 'warriors/sunset.red',
                     'dependencies/SDL3-3.4.16.zip', 'licenses/SDL3.txt', 'QUICKSTART.txt'):
            assert prefix + name in names, name
        expected_binaries = {prefix + 'bin/' + name for name in programs(target)}
        assert {name for name in names if name.startswith(prefix + 'bin/')} == expected_binaries
        for name in expected_binaries:
            assert archive.getinfo(name).external_attr >> 16 & stat.S_IXUSR, name
        metadata = json.loads(archive.read(prefix + 'release.json'))
        assert metadata['platform'] == target and metadata['version'] == ver
        if target == 'source':
            print('PASS: source ZIP contents, licenses and integrity')
            return
        with tempfile.TemporaryDirectory(prefix='pmars release ', dir=ROOT / 'security' / 'build') as folder:
            folder = Path(folder)
            if os.name == 'nt':
                archive.extractall(folder)
            else:
                # Test a real unzip utility's preservation of executable bits.
                subprocess.run(['unzip', '-q', str(path.resolve()), '-d', str(folder)], check=True)
            install = folder / prefix.rstrip('/')
            cwd = folder / 'unrelated'
            cwd.mkdir()
            warrior = cwd / 'probe.red'
            warrior.write_text(';redcode\n;assert CORESIZE == 80\ndat 0,0\n')
            env = dict(os.environ, SDL_VIDEO_DRIVER='dummy')
            env.pop('PMARSHOME', None)
            env['PATH'] = str(Path(os.environ['SystemRoot']) / 'System32') if os.name == 'nt' else '/usr/bin:/bin'
            baseline = None
            for name in programs(target):
                exe = install / 'bin' / name
                result = subprocess.run([str(exe), '-@', 'nano.opt', '-b', '-k', '-r', '1',
                                         '-c', '10', '-F', '20', str(warrior), str(warrior)],
                                        env=env, cwd=cwd, input=b'', capture_output=True, timeout=30)
                assert result.returncode == 0, (name, result)
                assert result.stdout, (name, 'Missing scores')
                if baseline is None:
                    baseline = result.stdout
                assert result.stdout == baseline, (name, 'Score mismatch', result.stdout, baseline)
            print(f'PASS: {len(programs(target))} packaged executables run from an unrelated directory; '
                  'config lookup and battle scores agree, without compiler/SDL runtime on PATH')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', required=True, choices=PLATFORMS)
    parser.add_argument('--output', type=Path, default=ROOT / 'dist')
    args = parser.parse_args()
    (ROOT / 'security' / 'build').mkdir(parents=True, exist_ok=True)
    ver = version()
    test_archive(args.output / archive_name(ver, args.platform), args.platform, ver)
