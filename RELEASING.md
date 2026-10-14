# Binary releases

The **Build release binaries** GitHub Actions workflow builds and tests:

| Package | Runner / compiler | Included binaries |
| --- | --- | --- |
| Windows x64 | Windows 2022, MSYS2 UCRT64 GCC | pmars.exe, pmars-server.exe, pmars-sdl3.exe, pmars-gdi.exe |
| Linux x64 | Ubuntu 22.04, GCC | pmars, pmars-server, pmars-sdl3 |
| macOS Apple Silicon | macOS 15 ARM64, Apple Clang | pmars, pmars-server, pmars-sdl3 |
| macOS Intel | macOS 15 Intel, Apple Clang | pmars, pmars-server, pmars-sdl3 |

`pmars` includes cdb; `pmars-server` excludes it. SDL3 and Windows compiler
runtimes are linked statically. Linux still needs glibc 2.35 or newer and
system display libraries. macOS builds target macOS 11 or newer; they are
not signed with a Developer ID or notarized. Test on the supported systems
before advertising a particular minimum OS as verified.

## Try a build

Pull requests and pushes to `main` build all four platforms without publishing
a release. After all jobs pass, download **release-downloads** from the workflow
run's Artifacts section. It contains five ZIPs and `SHA256SUMS`.

Once the workflow is on the default branch, you can also select
**Actions → Build release binaries → Run workflow** and choose a branch.
Manual runs produce artifacts only, even when run against a tag.
Untagged builds are named `dev-<12-character-commit>`; PR builds use the tested
merge commit. Artifact downloads follow the repository's retention setting.

## Publish a release

Merge the changes to `main` and wait for the full build matrix to pass. Then,
using the desired version number:

Keep `PMARSVER`/`PMARSDATE` in `src/global.h`, the CMake project version, and
release documentation current when changing the program version. The tag
names the release archive; it does not change the executable's version text.

```sh
git switch main
git pull --ff-only
git tag -a v0.9.6 -m "pMARS AI 0.9.6"
git push origin v0.9.6
```

Only a **pushed** `vMAJOR.MINOR.PATCH` tag (optionally with a prerelease suffix
such as `-rc.1`) publishes a release. Prerelease tags are marked as prereleases.
No release is created unless every build, regression suite, extracted-package
smoke test, and archive verification succeeds. The final job uses GitHub's
built-in token with `contents: write`; no personal token or secret is required.
Repository/organization policies must permit GitHub Actions and release writes.

The workflow creates these release assets:

```text
pmars-ai-0.9.6-source.zip
pmars-ai-0.9.6-windows-x64.zip
pmars-ai-0.9.6-linux-x64.zip
pmars-ai-0.9.6-macos-arm64.zip
pmars-ai-0.9.6-macos-x64.zip
SHA256SUMS
```

Each platform ZIP contains the complete matching Git source (including
warriors, config, documentation, tests, and bundled SDL3 source), plus its
binaries in `bin/`. The source ZIP has no binaries. Every ZIP has one
`pmars-ai-<version>/` root folder, a visible SDL3 license, `QUICKSTART.txt`,
and `release.json` recording the exact source commit and platform.
Unix executable permissions are recorded in the ZIP; use an unzip tool that
preserves them. Packages contain neither build intermediates nor `.git`.

The release description is generated from GitHub's release notes and can be
edited afterward. Existing releases/assets are never overwritten. A failed
upload may leave a draft release: inspect it before deleting that draft and
rerunning the failed job. A published release should get a new version/tag
when its contents need changing; rerunning its publish job fails deliberately.

## Local packaging

Build and test first, using the same commands/options as the workflow. On
Windows the console and server need `LDFLAGS=-static` for standalone packaging;
use a fresh build or rebuild existing objects/executables with the new flags.
Then run (Python 3.10+):

```sh
python scripts/package_release.py package --platform source
python scripts/package_release.py package --platform windows-x64
python scripts/test_release_package.py --platform windows-x64
```

Choose the native platform as appropriate. Output goes into ignored `dist/`.
Source comes from **Git HEAD**, not uncommitted working files: commit source
changes and rebuild before preparing a distributable release. The binary
header is checked against the requested platform/architecture.
`python scripts/package_release.py checksums` requires all five archives for
the current version and commit, and rejects missing, extra or mixed-version
archives. It is normally run by the collection job after downloading the
four native builds and source package.
