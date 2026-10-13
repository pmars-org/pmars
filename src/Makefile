# -------------------------------
# portable makefile for pMARS
# -------------------------------

# Usage:

#   make default        default build (no display, with cdb debugger)
#   make server         hardened server build (disable cdb debugger)
#   make curses         enable Curses display (UNIX)
#   make sdl            enable SDL graphics (generic)
#   make xwin           enable X Windows graphics (UNIX)
#   make check          run server regression tests (requires Python 3)
#   make clean          remove objects and generated executables
#                       (use before switching display builds)

# Run these commands in src. On Windows, GNU Make may be named mingw32-make.
# Python is needed only for make check; override it with PYTHON=path/to/python.
# Server output is pmars-server[.exe]; other builds use pmars[.exe].
#
# Config options (add to CFLAGS):

#   -DRWLIMIT           enables read/write limits
#   -DSAFECOLORS        use Okabe-Ito CVD-safe palette

CFLAGS += -O2 -DEXT94 -DPERMUTATE

CC = gcc                # required for Linux
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

.PHONY: all help curses default sdl server check xwin clean

all: help

help:
	@echo "-------------------------------"
	@echo "portable makefile for pMARS"
	@echo "-------------------------------"
	@echo "Usage:"
	@echo "  make default        default build (no display, with cdb debugger)"
	@echo "  make server         hardened server build (disable cdb debugger)"
	@echo "  make curses         enable Curses display (UNIX)"
	@echo "  make sdl            enable SDL graphics (generic)"
	@echo "  make xwin           enable X Windows graphics (UNIX)"
	@echo "  make check          run server regression tests (requires Python 3)"
	@echo "  make clean          remove objects and generated executables"
	@echo "                      (use before switching display builds)"

curses: CFLAGS += -DCURSESGRAPHX
curses: LIBS = -lncurses
curses: $(MAINFILE)

default: $(MAINFILE)

sdl: CFLAGS += -DSDLGRAPHX `sdl-config --cflags`
sdl: LIBS = `sdl-config --libs`
sdl: $(MAINFILE)

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

sim.o: curdisp.c uidisp.c sdldisp.c xwindisp.c

sdldisp.c: pmarsicnsdl.h

xwindisp.c: xwindisp.h pmarsicn.h

$(OBJ): Makefile config.h global.h

%.o: %.c
	@echo Compiling $<
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
ifeq ($(OS),Windows_NT)
	-cmd /c del /Q $(OBJ) core $(MAINFILE) $(SERVERFILE) "..\security\build\test-eval$(EXEEXT)" 2>NUL
else
	rm -f $(OBJ) core $(MAINFILE) $(SERVERFILE) ../security/build/test-eval$(EXEEXT)
endif

