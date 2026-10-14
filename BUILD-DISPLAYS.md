# Building the GDI and SDL3 displays

Run the following commands in `src`. GNU Make is named `mingw32-make` in
many Windows installations. Plain `make` prints the available targets.

| Command | Result | Requirements |
| --- | --- | --- |
| `make default` | Hardened console with debugger: `pmars` | C compiler and GNU Make |
| `make server` | Hardened headless simulator: `pmars-server` | C compiler and GNU Make |
| `make gdi` | Native Windows display: `pmars-gdi.exe` | 64-bit MinGW-w64 GCC and GNU Make |
| `make sdl` (or `make sdl3`) | Portable SDL3 display: `pmars-sdl3` | C/C++ compilers, GNU Make, CMake 3.21+ |

Windows executable names end in `.exe`. GDI needs no SDL or CMake.
By default, SDL3 source is bundled in `dependencies/SDL3-3.4.16.zip`; CMake checks its
SHA-256 and extracts it into the build directory. Builds do not download SDL.
The archive includes SDL's license. Native platform development libraries
are also required on Linux. Windows SDL3 builds require 64-bit MinGW-w64.
The default Windows display builds link compiler runtimes statically; the
default SDL3 build also links SDL3 statically. Python is required only for regression tests.

## Options

Use `CC=...`, `CXX=...`, `CMAKE=...`, `JOBS=...` (default 8),
`SDL_BUILD_DIR=...` (default `../build-sdl3`), `SDL_BUILD_TYPE=...`
(default Release), or `CMAKE_ARGS=...` to customize the SDL3 build.
`SDL_DIR=...` selects an existing SDL3 source directory. Paths are relative
to `src`; use a fresh build directory when changing compilers or platforms.
`CPPFLAGS="-DRWLIMIT -DSAFECOLORS"` enables optional core limits and palette.

All build targets are in `src/Makefile`. GDI and server builds compile their
sources separately from console/display objects; SDL3 has its own CMake
build directory. Run `make clean` before switching between the legacy
`default`, `curses` and `xwin` targets. Cleaning preserves extracted SDL source.

## Executable symbols and debugging pMARS

GNU Make strips symbols from the generated console, server, GDI and SDL3
executables by default. Set `NO_STRIPPING=1` to keep them, and use `STRIP=...`
to select the strip tool when cross-compiling. This does not disable cdb:
the Redcode debugger remains available in console and graphics builds.

Keeping symbols and generating debug information are separate settings.
For a console build suitable for a native debugger such as GDB, rebuild with:

```sh
make -C src -B default NO_STRIPPING=1 CFLAGS="-O0 -g -std=c99 -DEXT94 -DPERMUTATE"
```

Use `server` or `gdi` instead of `default` for those variants. For SDL3:

```sh
make -C src sdl NO_STRIPPING=1 SDL_BUILD_TYPE=Debug SDL_BUILD_DIR=../build-sdl3-debug \
  CFLAGS="-O0 -g -std=c99 -DEXT94 -DPERMUTATE"
```

Use `SDL_BUILD_TYPE=RelWithDebInfo` for an optimized SDL build with debug
information. With direct CMake builds, use `-DCMAKE_BUILD_TYPE=Debug` or
`RelWithDebInfo`; direct CMake build outputs are not stripped. GNU Make strips
only the copied SDL executable in `src/`, preserving the CMake build output.
When changing `NO_STRIPPING` or compiler flags for the console, use `-B` to
rebuild: stripping cannot restore symbols to an existing executable.

## Tests

- `make check`: headless security, compatibility and expression tests.
- `make check-console`: console security and compatibility tests, battle parity
  against the server, and console/server debugger behavior checks.
- `make check-gdi`: GDI security, assembler, battle parity, debugger and
  surface tests. This opens short-lived Windows windows.
- `make check-sdl`: SDL3 security, assembler, battle parity, debugger and
  event tests with the dummy video driver. On Windows it also checks DLL
  imports and launching without SDL/compiler runtime DLLs.
- `make check-sdl-native`: additionally exercises native SDL3 windows/input.

Python 3 runs the harnesses; Windows SDL3 import checks also use MinGW objdump.
GitHub Actions runs headless, console and display regressions on Linux and Windows
(GDI only on Windows). Native SDL3 desktop checks are run manually.

## Run

### Finding option files and debugger macros

For example, `pmars -@ nano.opt warrior1.red warrior2.red` works from any
working directory when the executable is on PATH and the presets are installed
in one of the locations below. Bare filenames for `-@` and cdb macro files
(including `pmars.mac`) are searched in this order:

1. The current working directory.
2. `PMARSHOME`, if set to a nonempty directory.
3. `<executable-directory>/../config` (portable ZIP or source-tree builds).
4. `<executable-directory>/../share/pmars` (standard package layout).
5. The executable directory itself (flat portable packages).
6. The compiled `PMARS_DATADIR`, if configured.

The actual executable location is used, independently of the current directory;
Linux/macOS symlinks resolve to the installed executable. Windows launchers such
as Chocolatey shims therefore use the program's own directory, not the shim's.
Names containing a directory separator or colon are treated as explicit paths
and are never searched elsewhere. Unreadable files do not silently fall back
to a different preset. Nested `-@` includes use the same lookup, with the
existing depth limit. Warrior filenames retain their existing path behavior.
No option preset is loaded automatically: select it with `-@ filename.opt`.

CMake installs the `.opt` and `.mac` files to `${CMAKE_INSTALL_DATADIR}/pmars`
and compiles `${CMAKE_INSTALL_FULL_DATADIR}/pmars` as the last fallback.
Package recipes can override `-DPMARS_DATADIR=/custom/share/pmars`; configure
the final install prefix before building and use DESTDIR for staging.
GNU Make builds can set `PMARS_DATADIR=/custom/share/pmars` too; clean existing
objects when changing this setting. Use forward slashes for Windows paths.

From the repository root on Windows:

```powershell
.\src\pmars-gdi.exe -v 834 warriors\sunset.red warriors\excalibur.red
.\src\pmars-sdl3.exe -v 834 warriors\sunset.red warriors\excalibur.red
```

Add `-e` for the debugger or `-m 960x720` to select the window size.
See BUILD-WINDOWS-GDI.md and BUILD-WINDOWS-SDL.md for controls and details.

## Port scope

The GDI adapter uses `gdicore.c` with private surface/event definitions in
`gdidisp.c` and `gdidisp.h`. `sdldisp.c` implements SDL3. Both use the existing
pMARS simulation/debugger interfaces and preserve the upstream palette.
The ports also expose the `-m` display mode option, handle redirected input,
and initialize the graphical A-address mark when RWLIMIT is disabled.
The later missing-errno-header fix is included for macOS compilation.
Browser/WebAssembly extensions are not included.

## Validation of this import (2026-10-13)

Windows x64 with MinGW-w64 GCC: console, hardened server, GDI and bundled
SDL3 builds passed. Core checks: 264 security/compatibility, 7 assembler
and 25,793 expression checks. Each display passed 276 security/compatibility
checks against the server and 7 assembler checks; GDI passed 30 mode/debugger
checks plus the surface suite, and SDL3 passed 18 mode/debugger checks.
SDL3 dummy and native Windows event tests, static-import checks and
clean-directory launches passed. Linux CI and macOS builds for this import
remain unverified; macOS is not yet included in the workflow.

## System SDL3 and packaging

Package managers can use their installed SDL3 development package (version
3.2 or later), without extracting, building or downloading bundled SDL3:

```sh
make -C src sdl SDL_PROVIDER=system
make -C src check-sdl SDL_PROVIDER=system
```

System mode defaults to **shared** SDL3. Set `SDL_LINK=static` explicitly
if the installed package provides the static library instead. Bundled mode
defaults to static; `SDL_LINK=shared` is also supported. These choices use
separate default build directories. Shared builds require the SDL3 runtime
library to be available at launch (on Windows, SDL3.dll and any compiler
runtime DLLs must be beside the executable or on PATH).

For a package recipe, CMake can be used directly:

```sh
cmake -S . -B build-package -DPMARS_SDL_PROVIDER=system -DPMARS_STATIC=OFF \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build-package --target pmars-sdl3
DESTDIR="$pkgdir" cmake --install build-package
```

The install rule installs `bin/pmars-sdl3` and the option/macro files under
`share/pmars` (or the configured GNUInstallDirs paths); the package supplies SDL3 as a
runtime dependency. Use `CMAKE_PREFIX_PATH` or CMake's `SDL3_DIR` to locate
an SDK outside the normal search paths. With make, pass these through
`CMAKE_ARGS`. `SDL_DIR` is only for source trees in bundled mode.
System mode never falls back to the bundled archive if SDL3 is missing.
Use a fresh build directory when changing CMake provider/linkage options.
The Windows standalone-DLL checks apply only to static builds; shared builds
check the SDL3 DLL import and still run the display and compatibility tests.

System-provider validation on Windows: built a separate shared SDL3 SDK,
then configured pMARS against its installed CMake package. All 276
security/compatibility, 18 SDL mode/debugger, 7 assembler and dummy event
checks passed; the executable imports SDL3.dll. The install rule staged
only bin/pmars-sdl3.exe. Missing-SDK configuration failed without extracting
the bundled archive. The default bundled-static suite also passed again.

## High-DPI displays

GDI and SDL3 scale the bitmap UI to the desktop display scale. At 150%,
`-m 640x480` requests a 960x720-pixel client area on Windows, while retaining
640x480 logical drawing coordinates. Text, core cells and mouse input scale
together. Resizing changes the logical layout; moving between displays updates
the scale. The historical bitmap font uses nearest-neighbor enlargement, so
fractional scales can produce uneven pixel widths.
