/* Native Win32 driver for the shared pMARS bitmap renderer. GPL-2.0-or-later.
 * This private subset retains SDL-style names to reuse sdldisp.c; it is NOT
 * an SDL implementation or ABI and needs no SDL headers or libraries.
 */
#ifndef PMARS_GDIDISP_H
#define PMARS_GDIDISP_H
#include <stdint.h>
typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef int16_t Sint16;
typedef uint32_t Uint32;
typedef struct { Sint16 x, y; Uint16 w, h; } SDL_Rect;
typedef struct {
    int BitsPerPixel, BytesPerPixel;
    Uint32 Rmask, Gmask, Bmask;
} SDL_PixelFormat;
typedef struct {
    int w, h, pitch;
    Uint32 flags, key;
    void *pixels;
    SDL_PixelFormat *format;
    SDL_Rect clip;
} SDL_Surface;
typedef struct { int blit_fill, video_mem; } SDL_VideoInfo;
typedef struct { int sym, mod; Uint16 unicode; } SDL_keysym;
typedef struct {
    int type;
    struct { SDL_keysym keysym; } key;
    struct { int w, h; } resize;
    struct { int button, x, y; } button;
} SDL_Event;
enum { SDL_QUIT=1, SDL_KEYDOWN, SDL_MOUSEBUTTONDOWN, SDL_VIDEORESIZE, SDL_VIDEOEXPOSE };
enum { SDL_SWSURFACE=0, SDL_FULLSCREEN=1, SDL_RESIZABLE=2, SDL_ANYFORMAT=4,
       SDL_NOFRAME=8, SDL_DOUBLEBUF=16, SDL_HWSURFACE=32,
       SDL_SRCCOLORKEY=64, SDL_RLEACCEL=128, SDL_INIT_VIDEO=256 };
enum { KMOD_SHIFT=1, KMOD_CTRL=2, KMOD_LALT=4, KMOD_RALT=8,
       KMOD_ALT=12, KMOD_META=16, KMOD_MODE=32, KMOD_NUM=64 };
enum { SDLK_UNKNOWN=0, SDLK_BACKSPACE=8, SDLK_TAB=9, SDLK_RETURN=13,
       SDLK_ESCAPE=27, SDLK_c='c', SDLK_DELETE=127,
       SDLK_UP=256, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_HOME, SDLK_END,
       SDLK_INSERT, SDLK_PAGEUP, SDLK_PAGEDOWN, SDLK_BREAK, SDLK_HELP,
       SDLK_KP0, SDLK_KP1, SDLK_KP2, SDLK_KP3, SDLK_KP4, SDLK_KP5,
       SDLK_KP6, SDLK_KP7, SDLK_KP8, SDLK_KP9, SDLK_KP_ENTER,
       SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7,
       SDLK_F8, SDLK_F9, SDLK_F10, SDLK_F11, SDLK_F12, SDLK_F13, SDLK_F14, SDLK_F15 };
#define SDL_BYTEORDER 1234
#define SDL_BIG_ENDIAN 4321
#define SDL_MUSTLOCK(s) 0
#define SDL_LockSurface(s) 0
#define SDL_UnlockSurface(s) ((void)0)
#define SDL_DEFAULT_REPEAT_DELAY 0
#define SDL_DEFAULT_REPEAT_INTERVAL 0
#define SDL_EnableKeyRepeat(a,b) ((void)0)
#define SDL_EnableUNICODE(a) ((void)0)
int SDL_Init(Uint32 flags);
void SDL_Quit(void);
const char *SDL_GetError(void);
SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int bpp,
                                  Uint32 r, Uint32 g, Uint32 b, Uint32 a);
void SDL_FreeSurface(SDL_Surface *s);
SDL_Surface *SDL_DisplayFormat(SDL_Surface *s);
Uint32 SDL_MapRGB(SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b);
int SDL_SetColorKey(SDL_Surface *s, Uint32 flags, Uint32 key);
int SDL_SetClipRect(SDL_Surface *s, const SDL_Rect *r);
void SDL_GetClipRect(SDL_Surface *s, SDL_Rect *r);
int SDL_FillRect(SDL_Surface *s, SDL_Rect *r, Uint32 color);
int SDL_BlitSurface(SDL_Surface *s, SDL_Rect *sr, SDL_Surface *d, SDL_Rect *dr);
SDL_Surface *SDL_SetVideoMode(int w, int h, int bpp, Uint32 flags);
const SDL_VideoInfo *SDL_GetVideoInfo(void);
SDL_Rect **SDL_ListModes(SDL_PixelFormat *f, Uint32 flags);
void SDL_WM_SetCaption(const char *title, const char *icon);
int SDL_WM_ToggleFullScreen(SDL_Surface *s);
void SDL_UpdateRect(SDL_Surface *s, int x, int y, int w, int h);
int SDL_Flip(SDL_Surface *s);
int SDL_PollEvent(SDL_Event *e);
int SDL_WaitEvent(SDL_Event *e);
void SDL_WarpMouse(int x, int y);
void SDL_Delay(Uint32 ms);
#endif
