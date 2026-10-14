"""Create release ZIPs from Git HEAD and native build outputs (stdlib only)."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import re
import stat
import struct
import subprocess
import tarfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
PLATFORMS = ('source', 'windows-x64', 'linux-x64', 'macos-arm64', 'macos-x64')


def git(*args):
    return subprocess.check_output(['git', *args], cwd=ROOT)


def version():
    if os.environ.get('GITHUB_REF_TYPE') == 'tag':
        tag = os.environ['GITHUB_REF_NAME']
        if not re.fullmatch(r'v[0-9]+\.[0-9]+\.[0-9]+(?:-[A-Za-z0-9][A-Za-z0-9.-]*)?', tag):
            raise ValueError('Release tag must be vMAJOR.MINOR.PATCH, optionally with a prerelease suffix')
        return tag[1:]
    return 'dev-' + git('rev-parse', '--short=12', 'HEAD').decode().strip()


def archive_name(ver, target):
    return f'pmars-ai-{ver}-{target}.zip'


def programs(target):
    if target == 'source':
        return []
    names = ['pmars', 'pmars-server', 'pmars-sdl3']
    if target == 'windows-x64':
        names.append('pmars-gdi')
        return [name + '.exe' for name in names]
    return names


def check_architecture(data, target):
    if target == 'windows-x64':
        if data[:2] != b'MZ' or len(data) < 64:
            raise ValueError('Not a Windows executable')
        offset = struct.unpack_from('<I', data, 60)[0]
        valid = data[offset:offset + 4] == b'PE\0\0' and struct.unpack_from('<H', data, offset + 4)[0] == 0x8664
    elif target == 'linux-x64':
        valid = data[:6] == b'\x7fELF\x02\x01' and struct.unpack_from('<H', data, 18)[0] == 62
    else:
        cpu = 0x100000C if target == 'macos-arm64' else 0x1000007
        valid = data[:4] == b'\xcf\xfa\xed\xfe' and struct.unpack_from('<I', data, 4)[0] == cpu
    if not valid:
        raise ValueError(f'Executable does not match {target}')


def add_file(archive, name, data, executable=False):
    # Stable timestamps and Unix modes: unzip on Linux/macOS retains +x.
    entry = zipfile.ZipInfo(name, (2000, 1, 1, 0, 0, 0))
    entry.create_system = 3
    entry.external_attr = (stat.S_IFREG | (0o755 if executable else 0o644)) << 16
    entry.compress_type = zipfile.ZIP_DEFLATED
    archive.writestr(entry, data)


def package(output, ver, target):
    output.mkdir(parents=True, exist_ok=True)
    prefix = f'pmars-ai-{ver}/'
    destination = output / archive_name(ver, target)
    # Git's archive contains exactly the recorded source, never local binaries,
    # build trees, credentials, or unrelated untracked files.
    source = git('archive', '--format=tar', 'HEAD')
    sha = git('rev-parse', 'HEAD').decode().strip()
    with zipfile.ZipFile(destination, 'w') as archive:
        with tarfile.open(fileobj=io.BytesIO(source)) as tree:
            for member in tree:
                if member.isfile():
                    add_file(archive, prefix + member.name, tree.extractfile(member).read(), bool(member.mode & 0o111))
                elif not member.isdir():
                    raise ValueError(f'Unsupported source archive entry: {member.name}')
        for name in programs(target):
            data = (ROOT / 'src' / name).read_bytes()
            check_architecture(data, target)
            add_file(archive, prefix + 'bin/' + name, data, executable=True)
        # Make SDL's license visible without unpacking its included source ZIP.
        with zipfile.ZipFile(io.BytesIO(archive.read(prefix + 'dependencies/SDL3-3.4.16.zip'))) as sdl:
            add_file(archive, prefix + 'licenses/SDL3.txt', sdl.read('SDL3-3.4.16/LICENSE.txt'))
        metadata = dict(version=ver, platform=target, commit=sha, binaries=programs(target))
        add_file(archive, prefix + 'release.json', json.dumps(metadata, indent=2).encode() + b'\n')
        add_file(archive, prefix + 'QUICKSTART.txt', (
            'pMARS AI ' + ver + '\n\n'
            'Source: src/   Presets and debugger macros: config/   Examples: warriors/\n'
            'Build instructions: BUILD-DISPLAYS.md\n'
            'Platform packages include executables in bin/. Add that directory to PATH.\n'
            'pmars includes cdb; pmars-server disables the debugger for unattended runs.\n'
            'pmars-sdl3 provides graphics; Windows also includes pmars-gdi.\n'
            'From this directory (append .exe on Windows):\n'
            '  bin/pmars -@ 94nop.opt warriors/sunset.red warriors/excalibur.red\n'
            'From elsewhere: pmars -@ 94nop.opt <path-to-warrior1> <path-to-warrior2>\n'
            'Linux binaries require glibc 2.35 or newer and system display libraries.\n'
            'macOS binaries target macOS 11 or newer and are not notarized.\n'
            'Windows binaries require 64-bit Windows and use system DLLs only.\n'
            'SDL3 is linked statically; its source and license are included.\n'
        ).encode())
    print(destination)


def checksums(output, ver):
    expected = {archive_name(ver, target) for target in PLATFORMS}
    actual = {path.name for path in output.glob('*.zip')}
    if actual != expected:
        raise ValueError(f'Archive set mismatch: missing={expected - actual}, extra={actual - expected}')
    sha = git('rev-parse', 'HEAD').decode().strip()
    lines = []
    for target in PLATFORMS:
        name = archive_name(ver, target)
        path = output / name
        with zipfile.ZipFile(path) as archive:
            metadata = json.loads(archive.read(f'pmars-ai-{ver}/release.json'))
            if metadata != dict(version=ver, platform=target, commit=sha, binaries=programs(target)):
                raise ValueError(f'Mismatched release metadata: {name}')
            if archive.testzip() is not None:
                raise ValueError(f'Corrupt ZIP: {name}')
        lines.append(f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {name}\n')
    (output / 'SHA256SUMS').write_text(''.join(sorted(lines)), encoding='ascii', newline='\n')
    print(output / 'SHA256SUMS')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('command', choices=('version', 'package', 'checksums'))
    parser.add_argument('--platform', choices=PLATFORMS, default='source')
    parser.add_argument('--output', type=Path, default=ROOT / 'dist')
    args = parser.parse_args()
    ver = version()
    if args.command == 'version':
        print(ver)
    elif args.command == 'package':
        package(args.output, ver, args.platform)
    else:
        checksums(args.output, ver)
