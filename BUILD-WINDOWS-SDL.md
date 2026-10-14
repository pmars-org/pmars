# Windows SDL3 build

The current source supports both GDI and SDL3. See [BUILD-DISPLAYS.md](BUILD-DISPLAYS.md)
for the makefile settings and comparison. SDL3 requires 64-bit MinGW-w64
GCC/G++, GNU make, CMake 3.21+, and bundled source or an installed SDL3 SDK.

```powershell
mingw32-make -C src sdl
.\src\pmars-sdl3.exe -v 834 warriors\sunset.red warriors\excalibur.red
```

The default output is `src/pmars-sdl3.exe`, with SDL3 and compiler runtimes
linked statically. For an installed shared SDL3 SDK, use
`mingw32-make -C src sdl SDL_PROVIDER=system`; SDL3.dll and any compiler
runtime DLLs must then be available beside the executable or on PATH.
The default static build needs no SDL DLLs. CMake finds the SDL3 source or
archive in `dependencies`; use `SDL_DIR=...` to override the source location.

To test (requires Python 3 and MinGW objdump):

```powershell
mingw32-make -C src check-sdl
mingw32-make -C src check-sdl-native
```

The second command also opens native Windows windows for input/event tests.
`mingw32-make -C src sdl` remains a shortcut for SDL3.
For the smaller Windows-only executable, use `make gdi` instead.
