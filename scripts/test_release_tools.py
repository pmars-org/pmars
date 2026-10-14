"""Check release gating: complete artifact sets, provenance, hashes and tags."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

import package_release as release


class ReleaseGateTests(unittest.TestCase):
    def setUp(self):
        build = release.ROOT / 'security' / 'build'
        build.mkdir(parents=True, exist_ok=True)
        self.folder = tempfile.TemporaryDirectory(dir=build)
        self.addCleanup(self.folder.cleanup)
        self.output = Path(self.folder.name)
        self.ver = '0.9.6'
        self.sha = 'a' * 40
        self.git = patch.object(release, 'git', return_value=(self.sha + '\n').encode())
        self.git.start()
        self.addCleanup(self.git.stop)
        for target in release.PLATFORMS:
            self.write_archive(target)

    def write_archive(self, target, **overrides):
        metadata = dict(version=self.ver, platform=target, commit=self.sha, binaries=release.programs(target))
        metadata.update(overrides)
        path = self.output / release.archive_name(self.ver, target)
        with zipfile.ZipFile(path, 'w') as archive:
            archive.writestr(f'pmars-ai-{self.ver}/release.json', json.dumps(metadata))

    def test_complete_set_hashes_exact_archive_bytes(self):
        release.checksums(self.output, self.ver)
        lines = (self.output / 'SHA256SUMS').read_text().splitlines()
        self.assertEqual(len(lines), 5)
        for line in lines:
            digest, name = line.split('  ')
            self.assertEqual(digest, hashlib.sha256((self.output / name).read_bytes()).hexdigest())

    def test_missing_platform_blocks_publication(self):
        (self.output / release.archive_name(self.ver, 'macos-arm64')).unlink()
        with self.assertRaisesRegex(ValueError, 'Archive set mismatch'):
            release.checksums(self.output, self.ver)
        self.assertFalse((self.output / 'SHA256SUMS').exists())

    def test_extra_old_archive_blocks_publication(self):
        (self.output / 'old-release.zip').write_bytes(b'old')
        with self.assertRaisesRegex(ValueError, 'Archive set mismatch'):
            release.checksums(self.output, self.ver)

    def test_mixed_commit_version_or_platform_blocks_publication(self):
        for overrides in ({'commit': 'b' * 40}, {'version': '0.9.5'}, {'platform': 'linux-x64'}, {'binaries': []}):
            with self.subTest(overrides=overrides):
                self.write_archive('windows-x64', **overrides)
                with self.assertRaisesRegex(ValueError, 'Mismatched release metadata'):
                    release.checksums(self.output, self.ver)
        self.assertFalse((self.output / 'SHA256SUMS').exists())

    def test_tag_validation(self):
        for tag, expected in (('v0.9.6', '0.9.6'), ('v0.9.6-rc.1', '0.9.6-rc.1')):
            with patch.dict(release.os.environ, GITHUB_REF_TYPE='tag', GITHUB_REF_NAME=tag):
                self.assertEqual(release.version(), expected)
        for tag in ('vlatest', 'v1/../../bad', 'v1.2.3;echo bad'):
            with patch.dict(release.os.environ, GITHUB_REF_TYPE='tag', GITHUB_REF_NAME=tag):
                with self.assertRaises(ValueError):
                    release.version()


if __name__ == '__main__':
    unittest.main()
