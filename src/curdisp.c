/* pMARS -- a portable Memory Array Redcode Simulator
 * Copyright (C) 1993-1996 Albert Ma, Na'ndor Sieben, Stefan Strack and Mintardjo Wangsawidjaja
 * Copyright (C) 2000 Philip Kendall
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * curdisp.c: curses display.
 * $Id: curdisp.c,v 1.2 2000/12/24 12:51:48 iltzu Exp $
 */

#ifdef CURSESGRAPHX

#include <unistd.h>

#ifndef SIM_INCLUDED
#include "sim.h"
#endif

extern unsigned long loopDelayAr[SPEEDLEVELS];
static int use_color;

/* For window structure in BSD 4.4/Curses 8.x library */
#ifdef BSD44
#define _curx curx
#define _cury cury
#endif

typedef struct win_st {
  WINDOW *win;
  int     page;
  struct win_st *nextwin;
}       win_t;

#ifdef NEW_STYLE
extern void init_curses(void), end_curses(void);
extern void switch_page(int);
extern void escape(char *, char *, int);
extern void SetCursor(int, int);
extern void aputs5(char *, int);
extern char *agets5(char *, int, int);
extern void clear_page5(void);
extern char *ckey2macro(int ccode, char *buf);
extern void sighandler(int dummy);
extern void cur_display_init(void);
extern void text_panel_update(int), update_statusline(int);
extern win_t *createwindow(int, int, int, int, int);
extern void closewindow(int), winupdate(void);
#else
extern void init_curses(), end_curses();
extern void escape();
extern void SetCursor(), aputs5(), clear_page5();
extern char *agets5(), *ckey2macro();
extern void sighandler();
extern void cur_display_init(), switch_page(), text_panel_update();
extern void closewindow(), winupdate(), update_statusline();
extern win_t *createwindow();
#endif

/* strings */
extern char *cannotAllocateVirtualScreen;
extern char *cannotOpenNewWindow;
extern char *errorInCurdispc;
extern char *cannotAllocateWindowMemory;
extern char *cursesError;
extern char *pressAnyKey;

extern int curPanel;

static win_t *globalwin = NULL;
/* Can be accessed from curwinstat -> win. For speed purpose only */
static WINDOW *curwin = NULL, *corewin, *corewin2, *cdbwin, *cdbwin2;
static int scale;                /* to reduce computation time */
static char statusLine[120];
static int refreshCounter;

#define GRAPH_ERROR escape(cannotOpenNewWindow, "?()", GRAPH_ERR)
#define PABORT      escape(errorInCurdispc, "", GRAPH_ERR)

#define PUT_ARENA(addr, chr) \
do {\
  int i = ((addr) / scale); \
  int w = (W-warrior) % 8; \
  if (use_color) {\
    wattron(corewin, COLOR_PAIR(w + 1)); \
    mvwaddch(corewin, i / COLS, i % COLS, (chr)); \
    wattroff(corewin, COLOR_PAIR(w + 1)); \
  } else { \
    mvwaddch(corewin, i / COLS, i % COLS, (chr)); \
  } \
} while (0)

#define PUT_ARENA_INV(addr, chr) \
do {\
  int i = ((addr) / scale); \
  int w = (W-warrior) % 8; \
  if (use_color) {\
    wattron(corewin, COLOR_PAIR(w + 9)); \
    mvwaddch(corewin, i / COLS, i % COLS, (chr)); \
    wattroff(corewin, COLOR_PAIR(w + 9)); \
  } else { \
    wstandout(corewin); \
    mvwaddch(corewin, i / COLS, i % COLS, (chr)); \
    wstandend(corewin); \
  } \
} while (0)

#define display_die(warnum)
#define display_push(val)
#define display_init()      cur_display_init()
#define display_cycle()     cur_display_cycle()
#define display_clear()     text_display_clear()
#define display_read(addr) do { if (displayLevel > 3)\
        cur_display_read(addr); } while(0)
#define display_write(addr) do { if (displayLevel > 1)\
        cur_display_write(addr); } while(0)
#define display_dec(addr) do { if (displayLevel > 2)\
        cur_display_dec(addr); } while(0)
#define display_inc(addr) do { if (displayLevel > 2)\
        cur_display_inc(addr); } while(0)
#define display_exec(addr) do { if (displayLevel > 0)\
        cur_display_exec(addr); } while(0)
#define display_spl(warrior,tasks)
#define display_dat(addr,warrior,tasks) do { if (displayLevel > 0)\
        cur_display_dat(addr,warrior,tasks); } while(0)
#define display_close()     text_display_close()

#define cur_display_read(addr) \
 PUT_ARENA(addr, '.');
#define cur_display_write(addr) \
 PUT_ARENA(addr, '.');
#define cur_display_dec(addr) \
 PUT_ARENA(addr, '-');
#define cur_display_inc(addr) \
 PUT_ARENA(addr, '+');
#define cur_display_exec(addr) \
 PUT_ARENA_INV(addr, ((W-warrior)+'0'));
#define cur_display_spl(warrior,tasks)
#define cur_display_dat(addr,warrior,tasks) \
 PUT_ARENA(addr, '*');
#define cur_display_close() text_display_close()

void
cur_display_cycle()
{
  if (displayLevel && !--refreshCounter) {
    refreshCounter = refreshInterval;
    update_statusline(round_num);
    wrefresh(corewin);
  }

#if defined(SYSV) && defined(KEYPRESS)                         /* PAK */
  if (wgetch(corewin) != ERR)
    debugState = STEP;
#endif

  int loopDelay = loopDelayAr[displaySpeed];
  if (loopDelay > 1)
    usleep(loopDelay);
}

/* Initialize curses windows */
void
init_curses()
{
  initscr();
  use_color = has_colors();
    if (use_color) {
      start_color();
      use_default_colors();
#ifndef SAFECOLORS
      init_pair(1, COLOR_RED, COLOR_BLACK);
      init_pair(9, COLOR_BLACK, COLOR_RED);
      init_pair(2, COLOR_GREEN, COLOR_BLACK);
      init_pair(10, COLOR_BLACK, COLOR_GREEN);
      init_pair(3, COLOR_BLUE, COLOR_BLACK);
      init_pair(11, COLOR_BLACK, COLOR_BLUE);
      init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
      init_pair(12, COLOR_BLACK, COLOR_MAGENTA);
      init_pair(5, COLOR_YELLOW, COLOR_BLACK);
      init_pair(13, COLOR_BLACK, COLOR_YELLOW);
      init_pair(6, COLOR_CYAN, COLOR_BLACK);
      init_pair(14, COLOR_BLACK, COLOR_CYAN);
      init_pair(7, COLOR_WHITE, COLOR_BLACK);
      init_pair(15, COLOR_BLACK, COLOR_WHITE);
#else
      init_pair(1, COLOR_BLUE, COLOR_BLACK);
      init_pair(9, COLOR_BLACK, COLOR_BLUE);
      init_pair(2, COLOR_YELLOW, COLOR_BLACK);
      init_pair(10, COLOR_BLACK, COLOR_YELLOW);
      init_pair(3, COLOR_MAGENTA, COLOR_BLACK);
      init_pair(11, COLOR_BLACK, COLOR_MAGENTA);
      init_pair(4, COLOR_RED, COLOR_BLACK);
      init_pair(12, COLOR_BLACK, COLOR_RED);
      init_pair(5, COLOR_CYAN, COLOR_BLACK);
      init_pair(13, COLOR_BLACK, COLOR_CYAN);
      init_pair(6, COLOR_GREEN, COLOR_BLACK);
      init_pair(14, COLOR_BLACK, COLOR_GREEN);
      init_pair(7, COLOR_WHITE, COLOR_BLACK);
      init_pair(15, COLOR_BLACK, COLOR_WHITE);
#endif
    }

/* Handle all keys except interrupting keys */
  cbreak();

/* Do not echo the typed keys */
  noecho();

/* Do not use curses NL -> CR/LF or CR -> NL translation. Use linefeed cap
   if possible */
  nonl();

/* Handle keypad if it is supported */
#if defined(SYSV) && defined(KEYPRESS)                         /* PAK */
  keypad(stdscr, TRUE);
#endif
}

/* create a new window for specified page.
   All pages except CORE_PAGE are in non-interactive mode. */
win_t  *
createwindow(lines, cols, y, x, page)
  int     lines, cols, y, x, page;
{
  win_t  *awin;

  if ((awin = (win_t *) malloc(sizeof(win_t))) &&
      (awin->win = newwin(lines, cols, y, x))) {
    clearok(awin->win, TRUE);
    leaveok(awin->win, page == CORE_PAGE ? TRUE : FALSE);
    idlok(awin->win, page == CORE_PAGE ? FALSE : TRUE);
    scrollok(awin->win, page == CORE_PAGE ? FALSE : TRUE);
    awin->page = page;
    awin->nextwin = globalwin;
    globalwin = awin;
  } else
    escape(cannotAllocateWindowMemory, "getwin()", GRAPHERR);
  return awin;
}

int
newpg()
{
  int     pg = 0;
  win_t  *awin;

  do {
    switch (pg) {
    case CDB_PAGE:
    case CORE_PAGE:
    case DEF_PAGE:
    case HELP_PAGE:
      break;
    default:
      for (awin = globalwin; awin && awin->page != pg;
           awin = awin->nextwin);
      if (!awin)
        return pg;
    }
    pg++;
  } while (pg != -1);

  escape("Sorry, but I think the program is about to crash!", "newpg()",
         GRAPHERR);
}

void
cur_display_init()
{
  int     idx, statuslines, corelines;

  init_curses();

  switch (warriors) {
  case 1:
    statuslines = COLS < 40 ? 2 : 1;
    break;
  case 2:
    statuslines = COLS < 80 ? 2 : 1;
    break;
  default:
    statuslines = COLS < 60 ? 2 : 1;
  }
  corelines = LINES - statuslines;
  corewin = createwindow(corelines, COLS, 0, 0, CORE_PAGE)->win;
  corewin2 = createwindow(statuslines, COLS, corelines, 0, newpg())->win;
  cdbwin = createwindow(LINES, COLS, 0, 0, CDB_PAGE)->win;
  cdbwin2 = createwindow(LINES, COLS / 2, 0, COLS / 2, newpg())->win;
  refreshCounter = refreshInterval;
  scale = 1 + (coreSize - 1) / (corelines * COLS);
}

void
update_statusline(round_num)
  int     round_num;
{
  int xpos = 0;
  if (warriors < 3) {
    werase(corewin2);
    if (use_color)
      wattron(corewin2, COLOR_PAIR(1));
    mvwprintw(corewin2, 0, xpos, "%10.10s", warrior[0].name);
    if (use_color)
      wattroff(corewin2, COLOR_PAIR(1));
    xpos += 10;
    mvwprintw(corewin2, 0, xpos, " [0]: %-5d ", warrior[0].tasks);
    xpos += 12;
  }

  if (warriors == 2) {
    if (use_color)
      wattron(corewin2, COLOR_PAIR(2));
    mvwprintw(corewin2, 0, xpos, "%10.10s", warrior[1].name);
    if (use_color)
      wattroff(corewin2, COLOR_PAIR(2));
    xpos += 10;
    mvwprintw(corewin2, 0, xpos, " [1]: %-5d Cycle: %-6d R: %d/%d (%d %d %d)",
      warrior[1].tasks, cycle >> 1, round_num, rounds, warrior[0].score[0],
      warrior[0].score[2], warrior[0].score[1]);
  }

  if (warriors > 2)
    mvwprintw(corewin2, 0, xpos, "%d of %d warriors alive  Cycle: %-6d R: %d/%d",
      warriorsLeft, warriors, cycle / warriorsLeft, round_num, rounds);

  wrefresh(corewin2);
}

void
switch_page(page)
  int     page;
{
  win_t  *win;

  for (win = globalwin; win; win = win->nextwin)
    if (win->page == page)
      break;

  if (win)
    if (page == CDB_PAGE && curPanel) {
      touchwin(cdbwin);
      wrefresh(cdbwin);
      touchwin(cdbwin2);
      wrefresh(cdbwin2);
      curwin = curPanel == 1 ? cdbwin : cdbwin2;
      return;
    } else
      curwin = win->win;
  else
    curwin = createwindow(LINES, COLS, 0, 0, page)->win;

  touchwin(curwin);
  wrefresh(curwin);
}

void
closewin(page)
  int     page;
{
  win_t  *w, *p;
  for (p = NULL, w = globalwin; w; p = w, w = w->nextwin)
    if (w->page == page)
      break;

  if (w) {
    if (p)
      p->nextwin = w->nextwin;
    else
      globalwin = w->nextwin;
    delwin(w->win);
    free(w);
  }
}

/*
 * Open new window, centered, with horz+vert lines surrounding its edges No
 * update
 */
#define openCWin(lines, cols, page) \
 winUp(lines, cols, (LINES-(lines))/2, (COLS-(cols))/2, page)

void
text_panel_update(newpanel)
  int     newpanel;
{
  if (curPanel == newpanel)
    return;

  if (curPanel)
    if (newpanel)
      curwin = newpanel == 1 ? cdbwin : cdbwin2;
    else {
      wclear(cdbwin);
      wrefresh(cdbwin);
      closewin(CDB_PAGE);
      curwin = cdbwin = createwindow(LINES, COLS, 0, 0, CDB_PAGE)->win;
    }
  else {
    wclear(cdbwin);
    wrefresh(cdbwin);
    closewin(CDB_PAGE);
    cdbwin = createwindow(LINES, COLS / 2, 0, 0, CDB_PAGE)->win;

/* newpanel ought to be 1 or 2 */
    curwin = newpanel == 1 ? cdbwin : cdbwin2;
  }
  curPanel = newpanel;
}

void
clear_page5()
{
  wclear(curwin);
  wrefresh(curwin);
}

void
aputs5(str, attr)
  char   *str;
  int     attr;
{
  int     y, x;
  getyx(curwin, y, x);
  for (; *str; str++)
    waddch(curwin, (char) *str);
}

char   *
agets5(str, maxchar, attr)
  char   *str;
  int     attr;
  int     maxchar;
{
  if (inputRedirection) {
    return fgets(str, maxchar, stdin);
  } else {
    char    ch, f, *ostr = str;
    int     oy, ox;

    --maxchar;
    for (f = 1; maxchar && f;) {
      ch = wgetch(curwin);
      switch (ch) {
      case 127:                /* erase key on many terminals */
      case '\b':
        if (str > ostr) {
          str--;
          maxchar++;
          leaveok(curwin, TRUE);
          if (ox = getcurx(curwin)) {
            mvwaddch(curwin, getcury(curwin), --ox, ' ');
            wmove(curwin, getcury(curwin), ox);
          } else {
            oy = getcury(curwin) - 1;
            mvwaddch(curwin, oy, COLS - 1, ' ');
            wmove(curwin, oy, COLS - 1);
          }
          leaveok(curwin, FALSE);
          wrefresh(curwin);
        }
        break;
      case 27:
        leaveok(curwin, TRUE);
        for (getyx(curwin, oy, ox); str > ostr; str--, maxchar++) {
          if (ox--)
            mvwaddch(curwin, getcury(curwin), ox, ' ');
          else
          mvwaddch(curwin, oy, ox = COLS, ' ');
        }
        leaveok(curwin, FALSE);
        wmove(curwin, oy, ox);
        wrefresh(curwin);
        break;
      case '\r':
      case '\n':
        f = 0;
        break;
      default:
        if (ch < 32) {
#if 1
          aputs5(ckey2macro(ch, str), attr);
#else
          ckey2macro(ch, str);
          aputc5('\n', attr);
#endif
          wrefresh(curwin);
          return ostr;
        } else {
          waddch(curwin, *str++ = ch);
          maxchar--;
          wrefresh(curwin);
        }
      }
    }
    *(str++) = '\n';
    *str = '\0';
    waddch(curwin, '\n');
    wrefresh(curwin);
    return (ostr);
  }                                /* !inputRedirection */
}

/* must only be called from cdb() */
void
text_display_clear()
{
  if (use_color)
    wbkgdset(corewin, COLOR_PAIR(0));
  werase(corewin);
  wrefresh(corewin);
}

void
text_display_close()
{
  if (displayLevel) {
    update_statusline(round_num - 1);
    mvwaddstr(corewin, 0, 0, pressAnyKey);
    wrefresh(corewin);
    if (!inputRedirection) {
      getch();
    }
  }
  switch_page(DEF_PAGE);
  end_curses();
}

/*
 * The following proc attempts to serve as a graceful escape for unexpected
 * error such as insufficient memory allocation, etc It attempts to free all
 * memory, close all windows first and revert the previous tty mode + setting
 */

void
escape(s, p, errCode)
  char   *s, *p;
  int     errCode;
{
  clear();
  refresh();
  nl();
  nocbreak();
  echo();
  end_curses();

  if (errCode != SUCCESS)
    fprintf(stderr, "%s: %s\n%s\n", p, s, cursesError);
  else
    printf("%s\n", s);
  exit(errCode);
}

void
end_curses()
{
  if (curwin) {
    while (globalwin)
      closewin(globalwin->page);
    endwin();                        /* restore previous tty settings */
  }
}

void
winupdate()
{
  wrefresh(curwin);
}

void
SetCursor(x, y)
  int     x, y;
{
  wmove(curwin, y, x);
}
#endif
