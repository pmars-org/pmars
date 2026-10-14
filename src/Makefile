# -------------------------------
# portable makefile for pMARS
# -------------------------------

# Usage:

#   make default        default build (no display, with cdb debugger)
#   make server         hardened server build (disable cdb debugger)
#   make curses         enable Curses display (UNIX)
#   make gdi            native Windows GDI display (no SDL dependency)
#   make sdl            portable SDL3 display (requires CMake and C++)
#   make check-gdi      GDI regressions (opens Windows windows)
#   make check-sdl      SDL3 regressions (dummy video driver)
#   make check-sdl-native  also test native SDL3 windows and input
#   make xwin           enable X Windows graphics (UNIX)
#   make check          run server regression tests (requires Python 3)
#   make clean          remove objects and generated executables
#                       (use before switching display builds)

# Run these commands in src. On Windows, GNU Make may be named mingw32-make.
# Python is needed only for make check; override it with PYTHON=path/to/python.
# Outputs: pmars-server, pmars-gdi, pmars-sdl3, or pmars (with .exe on Windows).
# SDL3 uses the bundled source archive; SDL_DIR can select another source tree.
# See ../BUILD-DISPLAYS.md for dependencies and build settings.
#
# Config options (add to CFLAGS):

#   -DRWLIMIT           enables read/write limits
#   -DSAFECOLORS        use Okabe-Ito CVD-safe palette

CFLAGS += -O2 -DEXT94 -DPERMUTATE

CC = gcc                # override with CC=clang, etc.
CXX = g++
#CC = cc                # if you don't have gcc

# Keep server compilation separate from the interactive/display object files.
TARGET_TRIPLE := $(shell $(CC) -dumpmachine)
EXEEXT =
PYTHON = python3
HARDEN_CFLAGS = -fstack-protector-strong
HARDEN_LDFLAGS =
ifneq (,$(or $(findstring mingw,$(TARGET_TRIPLE)),$(findstring windows-gnu,$(TARGET_TRIPLE))))
EXEEXT = .exe
PYTHON = python
HARDEN_LDFLAGS += -Wl,--dynamicbase,--nxcompat
else
HARDEN_CFLAGS += -D_FORTIFY_SOURCE=2 -fPIE
# ELF linker flags do not apply to macOS or other non-Linux platforms.
ifneq (,$(findstring linux,$(TARGET_TRIPLE)))
HARDEN_LDFLAGS += -pie -Wl,-z,relro,-z,now
endif
endif

MAINFILE = pmars$(EXEEXT)
SERVERFILE = pmars-server$(EXEEXT)
SOURCES = pmars.c asm.c eval.c disasm.c cdb.c sim.c pos.c clparse.c global.c token.c str_eng.c

GDI_PROGRAM ?= pmars-gdi.exe
GDI_EXTRA_CFLAGS ?= -std=c99 -fstack-protector-strong
GDI_LDFLAGS ?= -static -Wl,--dynamicbase,--nxcompat
GDI_LIBS ?= -lgdi32 -luser32
CORE_SOURCES = $(SOURCES)

# SDL3 build settings, used on Windows, Linux and macOS.
# Paths are relative to src, even when make is run in the repository root.
# SDL_PROVIDER=bundled builds the archive; system uses an installed SDL3 package.
# SDL_LINK defaults to static for bundled and shared for system.
SDL_PROVIDER ?= bundled
SDL_LINK ?= $(if $(filter system,$(SDL_PROVIDER)),shared,static)
ifeq (,$(filter $(SDL_PROVIDER),bundled system))
$(error SDL_PROVIDER must be bundled or system)
endif
ifeq (,$(filter $(SDL_LINK),static shared))
$(error SDL_LINK must be static or shared)
endif
CMAKE ?= cmake
SDL_BUILD_DIR ?= $(if $(and $(filter bundled,$(SDL_PROVIDER)),$(filter static,$(SDL_LINK))),../build-sdl3,../build-sdl3-$(SDL_PROVIDER)-$(SDL_LINK))
ifneq (,$(EXEEXT))
CMAKE_GENERATOR ?= MinGW Makefiles
else
CMAKE_GENERATOR ?= Unix Makefiles
endif
SDL_BUILD_TYPE ?= Release
SDL_DIR ?=
SDL_PROGRAM ?= pmars-sdl3$(EXEEXT)
SDL_EXTRA_CFLAGS ?= -std=c99 -fstack-protector-strong
JOBS ?= 8
# Extra CMake options, e.g. CMAKE_ARGS=-DCMAKE_OSX_ARCHITECTURES=arm64
CMAKE_ARGS ?=
SDL_CONFIGURE = "$(CMAKE)" -S .. -B "$(SDL_BUILD_DIR)" -G "$(CMAKE_GENERATOR)" -DCMAKE_MAKE_PROGRAM="$(MAKE)" -DCMAKE_C_COMPILER="$(strip $(CC))" -DCMAKE_CXX_COMPILER="$(strip $(CXX))" -DCMAKE_BUILD_TYPE="$(SDL_BUILD_TYPE)" -DPMARS_SDL_PROVIDER=$(SDL_PROVIDER) -DPMARS_STATIC=$(if $(filter static,$(SDL_LINK)),ON,OFF) -DPMARS_BUILD_EVENT_TESTS=OFF -DSDL3_SOURCE_DIR="$(if $(SDL_DIR),$(abspath $(SDL_DIR)))" -DCMAKE_C_FLAGS="$(CPPFLAGS) $(CFLAGS) $(SDL_EXTRA_CFLAGS)" -DCMAKE_EXE_LINKER_FLAGS="$(LDFLAGS)" $(CMAKE_ARGS)

.PHONY: all help curses default sdl sdl3 gdi check-gdi check-sdl check-sdl3 check-sdl-native server check xwin clean

all: help

help:
	@echo "-------------------------------"
	@echo "portable makefile for pMARS"
	@echo "-------------------------------"
	@echo "Usage:"
	@echo "  make default        default build (no display, with cdb debugger)"
	@echo "  make server         hardened server build (disable cdb debugger)"
	@echo "  make curses         enable Curses display (UNIX)"
	@echo "  make gdi            native Windows GDI display (no SDL dependency)"
	@echo "  make sdl            bundled static SDL3 (requires CMake and C++)"
	@echo "  make sdl SDL_PROVIDER=system  use installed shared SDL3"
	@echo "  make check-gdi      GDI regressions (opens Windows windows)"
	@echo "  make check-sdl      SDL3 regressions (dummy video driver)"
	@echo "  make check-sdl-native  also test native SDL3 windows and input"
	@echo "  make xwin           enable X Windows graphics (UNIX)"
	@echo "  make check          run server regression tests (requires Python 3)"
	@echo "  make clean          remove objects and generated executables"
	@echo "                      (use before switching display builds)"

curses: CFLAGS += -DCURSESGRAPHX
curses: LIBS = -lncurses
curses: $(MAINFILE)

default: $(MAINFILE)

sdl3: sdl
check-sdl3: check-sdl

# Always compile the whole GDI variant: no objects leak between backends.
gdi:
ifneq (,$(EXEEXT))
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GDI_EXTRA_CFLAGS) -DGDIGRAPHX $(CORE_SOURCES) gdidisp.c $(LDFLAGS) $(GDI_LDFLAGS) $(GDI_LIBS) -o "$(GDI_PROGRAM)"
else
	$(error GDI requires a Windows-targeting compiler; use make sdl)
endif

check-gdi: gdi server
	$(PYTHON) ../security/test_gdi.py ./$(GDI_PROGRAM) ./$(SERVERFILE)
	$(PYTHON) ../security/test_096.py ./$(GDI_PROGRAM)
	$(PYTHON) -c "from pathlib import Path; Path('../security/build').mkdir(parents=True, exist_ok=True)"
	$(CC) -std=c99 -O2 -Wall -Wextra -Werror ../security/test_gdi_surface.c gdidisp.c $(GDI_LDFLAGS) $(GDI_LIBS) -o ../security/build/test-gdi-surface.exe
	$(PYTHON) -c "import subprocess; subprocess.run(['../security/build/test-gdi-surface.exe'], check=True)"
	$(CC) -std=c99 -O2 -Wall -Wextra -Werror ../security/test_gdi_dpi.c $(GDI_LDFLAGS) $(GDI_LIBS) -o ../security/build/test-gdi-dpi.exe
	$(PYTHON) -c "import subprocess; subprocess.run(['../security/build/test-gdi-dpi.exe'], check=True)"

# Isolated CMake output prevents reuse of another display backend's objects.
# The default embeds SDL3; SDL_PROVIDER=system uses the installed package.
sdl:
	$(SDL_CONFIGURE)
	"$(CMAKE)" --build "$(SDL_BUILD_DIR)" --target pmars-sdl3 --parallel $(JOBS)
	"$(CMAKE)" -E copy_if_different "$(SDL_BUILD_DIR)/pmars-sdl3$(EXEEXT)" "$(SDL_PROGRAM)"

check-sdl: sdl
	"$(CMAKE)" -S .. -B "$(SDL_BUILD_DIR)" -DPMARS_BUILD_EVENT_TESTS=ON
	"$(CMAKE)" --build "$(SDL_BUILD_DIR)" --target pmars-server pmars-sdl3-events --parallel $(JOBS)
	$(PYTHON) ../security/test_sdl3.py "$(SDL_BUILD_DIR)" $(SDL_TEST_FLAGS)

check-sdl-native: SDL_TEST_FLAGS = --native
check-sdl-native: check-sdl


# Compile all server sources together on every request: changing flags or
# switching from a display build must never reuse incompatible object files.
server:
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c99 -DSERVER $(HARDEN_CFLAGS) $(SOURCES) $(LDFLAGS) $(HARDEN_LDFLAGS) -o $(SERVERFILE)

check: server
	$(PYTHON) ../security/test_security.py ./$(SERVERFILE)
	$(PYTHON) ../security/test_096.py ./$(SERVERFILE)
	$(PYTHON) -c "from pathlib import Path; Path('../security/build').mkdir(parents=True, exist_ok=True)"
	$(CC) -std=c99 -O2 ../security/test_eval.c eval.c -o ../security/build/test-eval$(EXEEXT)
	$(PYTHON) -c "import subprocess; subprocess.run(['../security/build/test-eval$(EXEEXT)'], check=True)"

xwin: CFLAGS += -DXWINGRAPHX
xwin: LIBS = -L/usr/X11R6/lib -lX11
xwin: $(MAINFILE)

OBJ = pmars.o asm.o eval.o disasm.o cdb.o sim.o pos.o clparse.o global.o token.o str_eng.o

$(MAINFILE): $(OBJ)
	@echo Linking $@
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@strip $@
	@echo Done

token.o asm.o disasm.o: asm.h

sim.o cdb.o pos.o disasm.o: sim.h

sim.o: curdisp.c uidisp.c sdldisp.c gdicore.c gdidisp.h xwindisp.c

sdldisp.c: pmarsicnsdl.h

xwindisp.c: xwindisp.h pmarsicn.h

$(OBJ): Makefile config.h global.h

%.o: %.c
	@echo Compiling $<
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
ifeq ($(OS),Windows_NT)
	-cmd /c del /Q $(OBJ) core $(MAINFILE) $(SERVERFILE) $(GDI_PROGRAM) $(SDL_PROGRAM) "..\security\build\test-eval$(EXEEXT)" "..\security\build\test-gdi-surface.exe" 2>NUL
else
	rm -f $(OBJ) core $(MAINFILE) $(SERVERFILE) $(GDI_PROGRAM) $(SDL_PROGRAM) ../security/build/test-gdi-surface.exe ../security/build/test-eval$(EXEEXT)
endif

# Keep the SDL dependency cache, but clean its generated build outputs.
ifneq (,$(wildcard $(SDL_BUILD_DIR)/CMakeCache.txt))
	"$(CMAKE)" --build "$(SDL_BUILD_DIR)" --target clean
endif
