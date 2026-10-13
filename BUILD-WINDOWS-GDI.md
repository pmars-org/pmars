# Native Windows GDI build

Build from this directory with MinGW-w64 GCC and GNU make:

```powershell
mingw32-make -C src gdi
.\src\pmars-gdi.exe -v 804 warriors\sunset.red warriors\excalibur.red
```

The executable is written to `src/pmars-gdi.exe`. It uses only Windows system DLLs,
including the Universal C Runtime supplied with modern Windows. No SDL
headers, libraries, DLLs or installation are needed for the GDI build.
The console retains command-line output and exit codes.

To start in the graphical debugger:

```powershell
.\src\pmars-gdi.exe -e -m 960x720 -v 814 warriors\sunset.red warriors\excalibur.red
```

Select SDL3 with `mingw32-make -C src sdl`.
See [BUILD-DISPLAYS.md](BUILD-DISPLAYS.md) for dependencies and makefile
settings. The headless server build (`make server` in `src`) remains available.

## Controls

- Resize or maximize the window to recompute the core and debugger layout.
- Alt+Enter toggles borderless fullscreen on the current monitor, scaling
  the bitmap; toggling back restores the previous window rectangle.
- `<` increases speed; `>` decreases speed. `0` through `4` select detail.
- Space/R clears core marks. Q/Escape exits during simulation.
- Other ordinary keys during simulation enter the debugger. `-e` starts there.
- Type debugger commands such as `step`, `continue`, `switch 2`, `close`,
  `display 814`, and `quit`. Tab switches panels; Shift+Up/Down recalls history.
- Close, Alt+F4 and Ctrl+C exit immediately, including during the final key wait.
- Mouse buttons in the core invoke `mousel`, `mousem`, `mouser` debugger macros;
  function keys and navigation keys also use the existing macro system.

For macros, copy `config/pmars.mac` to `pmars.mac` in the working directory
and add bindings as desired. The supplied macro file does not necessarily
define every mouse/function-key binding. There is no clipboard-paste or
Unicode text editor: the retained pMARS debugger uses ASCII bitmap text.
Resizing clears prior debugger output, as in the shared SDL renderer;
the active input line and core marks are redrawn.

`-m` accepts the existing display syntax, e.g. `960x720`,
`640x480:fullscreen`, `640x480:noframe`, or `640x480:-resizable`.
GDI always renders to 32-bit pixels; legacy depth, hardware acceleration
and double-buffer hints do not select a different pixel format.
Surface dimensions are bounded to 8192 per axis.

Interactive battles wait for a key on completion. Redirected stdin supplies
debugger commands and suppresses that final wait. `-r 0` assembles without
opening a display. Double-clicking the executable without warrior arguments
only prints command-line help; use the commands above for a battle.

## Implementation

`GDIGRAPHX` selects `gdidisp.c` and its private header. `SOFTGRAPHX` enables
the shared renderer/debugger integration for either GDI or SDL. Selecting
both drivers is rejected, as is combining a graphical driver with SERVER.

The GDI renderer `gdicore.c` retains its original layout, palette, sprites, bitmap
font and debugger logic. The native adapter implements only the surface and
event operations that renderer uses. Its SDL-style names are private source
compatibility names, not a dependency on SDL or a general SDL replacement.
The candidate's assembler fixes and default disabled read/write limits are preserved.
The no-RWLIMIT graphical A-address initialization fix is included.

The native driver allocates top-down BGR pixel buffers and paints with
[StretchDIBits](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-stretchdibits).
It handles Win32 paint, resize, keyboard-layout-aware character input, mouse
coordinates, DPI awareness and borderless fullscreen. Painting is deferred
and coalesced by the Windows message queue; blocking debugger waits sleep
until input arrives. Native window headers are isolated from simulator code.

## Verification

Python 3 is needed for the tests:

```powershell
mingw32-make -C src check-gdi
python security\test_gdi.py src\pmars-gdi.exe src\pmars-server.exe
```

These tests open short-lived real Windows windows. They exercise core modes,
detail levels, window configurations, debugger panels and deterministic battle
results, plus the security suite. A separate C test verifies clipped fills,
overlapping scrolling, color-keyed blits, surface copies and allocation bounds.
The native adapter passes `-Wall -Wextra -Werror` with the tested GCC toolchain.

The 0.9.6 port is verified by automated Windows tests. Interactive keyboard,
mouse, fullscreen/resize and multi-monitor behavior need manual testing.
