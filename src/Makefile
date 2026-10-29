# -------------------------------
# generic UNIX makefile for pMARS
# -------------------------------

# Usage:

#   make default        default build (no display, with cdb debugger)
#   make server         server build (disable cdb debugger)
#   make curses         enable Curses display (UNIX)
#   make sdl            enable SDL graphics (generic)
#   make sdlwin         enable SDL graphics (Windows, MinGW, console output)
#   make xwin          enable X Windows graphics (UNIX)
#   make clean          remove all *.o (recommended when changing
#                       CFLAGS in Makefile or switching build version)

# Config options (add to CFLAGS):

#   -DRWLIMIT           enables read/write limits
#   -DSAFECOLORS        use Okabe-Ito CVD-safe palette

CFLAGS += -O2 -DEXT94 -DPERMUTATE

CC = gcc                # required for Linux
#CC = cc                # if you don't have gcc

MAINFILE = pmars

ifneq ($(filter Windows_NT,$(OS))$(filter MINGW% MSYS% CYGWIN%,$(shell uname -s)),)
EXE = .exe
endif

.PHONY: all curses default sdl sdlwin server xwin clean

all:
	@echo "-------------------------------"
	@echo "generic UNIX makefile for pMARS"
	@echo "-------------------------------"
	@echo "Usage:"
	@echo "  make default        default build (no display, with cdb debugger)"
	@echo "  make server         server build (disable cdb debugger)"
	@echo "  make curses         enable Curses display (UNIX)"
	@echo "  make sdl            enable SDL graphics (generic)"
	@echo "  make sdlwin         enable SDL graphics (Windows, MinGW, console output)"
	@echo "  make xwin          enable X Windows graphics (UNIX)"
	@echo "  make clean          remove all *.o (recommended when changing"
	@echo "                      CFLAGS in Makefile or switching build version)"

curses: CFLAGS += -DCURSESGRAPHX
curses: LIBS = -lncurses
curses: $(MAINFILE)

default: $(MAINFILE)

sdl: CFLAGS += -DSDLGRAPHX `sdl-config --cflags`
sdl: LIBS = `sdl-config --libs`
sdl: $(MAINFILE)

# Link our SDL_main.c instead of libSDLmain, which redirects output to
# stdout.txt / stderr.txt, and build a console program instead of -mwindows
sdlwin: CFLAGS += -DSDLGRAPHX `sdl-config --cflags`
sdlwin: LIBS = SDL_main.o $(filter-out -lSDLmain -mwindows,$(shell sdl-config --libs)) -mconsole
sdlwin: SDL_main.o $(MAINFILE)

server: CFLAGS += -DSERVER
server: LIBS =
server: $(MAINFILE)

xwin: CFLAGS += -DXWINGRAPHX
xwin: LIBS = -L/usr/X11R6/lib -lX11
xwin: $(MAINFILE)

OBJ = pmars.o asm.o eval.o disasm.o cdb.o sim.o pos.o clparse.o global.o token.o str_eng.o

$(MAINFILE): $(OBJ)
	@echo Linking $@
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@strip $@$(EXE)
	@echo Done

token.o asm.o disasm.o: asm.h

sim.o cdb.o pos.o disasm.o: sim.h

sim.o: curdisp.c uidisp.c sdldisp.c xwindisp.c

sdldisp.c: pmarsicnsdl.h

xwindisp.c: xwindisp.h pmarsicn.h

$(OBJ) SDL_main.o: Makefile config.h global.h

%.o: %.c
	@echo Compiling $<
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) SDL_main.o core

