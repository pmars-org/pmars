/* pMARS native Windows surface/event driver. GPL-2.0-or-later.
 * SDL-style entry points are a private adapter for the existing renderer.
 * All surfaces are top-down 32-bit BGR DIBs. Windows owns only the window;
 * pMARS owns the pixel buffers, sprites, palette, font and debugger panels.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "gdidisp.h"

static SDL_PixelFormat format = {32, 4, 0xff0000, 0xff00, 0xff};
static HWND window;
static SDL_Surface *screen;
static char error_text[256];
static int changing_mode, fullscreen;
static DWORD window_style;
static RECT saved_window;
static SDL_Event events[256];
static unsigned read_event, write_event;
static const WCHAR window_class[] = L"pMARS_GDI";

static void win_error(const char *what)
{
    snprintf(error_text, sizeof(error_text), "%s (Windows error %lu)", what,
             (unsigned long)GetLastError());
}

const char *SDL_GetError(void) { return error_text; }

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int w, int h, int bpp,
                                  Uint32 r, Uint32 g, Uint32 b, Uint32 a)
{
    SDL_Surface *s;
    (void)bpp; (void)r; (void)g; (void)b; (void)a;
    /* Keep coordinates representable in the shared renderer; cap allocation. */
    if (w < 1 || h < 1 || w > 8192 || h > 8192) {
        strcpy(error_text, "Surface dimensions must be between 1 and 8192");
        return NULL;
    }
    s = calloc(1, sizeof(*s));
    if (s) s->pixels = calloc((size_t)w * h, 4);
    if (!s || !s->pixels) {
        free(s);
        strcpy(error_text, "Cannot allocate pixel buffer");
        return NULL;
    }
    s->w=w; s->h=h; s->pitch=w*4; s->format=&format; s->flags=flags;
    s->clip.w=(Uint16)w; s->clip.h=(Uint16)h;
    return s;
}

void SDL_FreeSurface(SDL_Surface *s)
{
    if (!s) return;
    if (s == screen) screen = NULL;
    free(s->pixels);
    free(s);
}

SDL_Surface *SDL_DisplayFormat(SDL_Surface *s)
{
    SDL_Surface *copy = SDL_CreateRGBSurface(s->flags,s->w,s->h,32,0,0,0,0);
    if (copy) {
        memcpy(copy->pixels,s->pixels,(size_t)s->pitch*s->h);
        copy->key=s->key;
        copy->clip=s->clip;
    }
    return copy;
}

Uint32 SDL_MapRGB(SDL_PixelFormat *f, Uint8 r, Uint8 g, Uint8 b)
{ (void)f; return ((Uint32)r<<16) | ((Uint32)g<<8) | b; }

int SDL_SetColorKey(SDL_Surface *s, Uint32 flags, Uint32 key)
{
    s->flags=(s->flags & ~SDL_SRCCOLORKEY) | (flags & SDL_SRCCOLORKEY);
    s->key=key;
    return 0;
}

int SDL_SetClipRect(SDL_Surface *s, const SDL_Rect *r)
{
    int x=0,y=0,right=s->w,bottom=s->h;
    if (r) {
        x=max(0,r->x); y=max(0,r->y);
        right=min(s->w,r->x+r->w); bottom=min(s->h,r->y+r->h);
    }
    s->clip.x=(Sint16)x; s->clip.y=(Sint16)y;
    s->clip.w=(Uint16)max(0,right-x); s->clip.h=(Uint16)max(0,bottom-y);
    return s->clip.w && s->clip.h;
}

void SDL_GetClipRect(SDL_Surface *s, SDL_Rect *r) { *r=s->clip; }

int SDL_FillRect(SDL_Surface *s, SDL_Rect *r, Uint32 color)
{
    int x,y,l=s->clip.x,t=s->clip.y;
    int right=l+s->clip.w,bottom=t+s->clip.h;
    if (r) {
        l=max(l,r->x); t=max(t,r->y);
        right=min(right,r->x+r->w); bottom=min(bottom,r->y+r->h);
    }
    for (y=t;y<bottom;y++) {
        Uint32 *row=(Uint32 *)s->pixels+(size_t)y*s->w;
        for (x=l;x<right;x++) row[x]=color;
    }
    return 0;
}

int SDL_BlitSurface(SDL_Surface *s, SDL_Rect *sr, SDL_Surface *d, SDL_Rect *dr)
{
    int sx=sr?sr->x:0, sy=sr?sr->y:0;
    int w=sr?sr->w:s->w, h=sr?sr->h:s->h;
    int dx=dr?dr->x:0, dy=dr?dr->y:0;
    int trim,x,y,start,end,step;
    trim=max(max(-sx,d->clip.x-dx),0); sx+=trim; dx+=trim; w-=trim;
    trim=max(max(-sy,d->clip.y-dy),0); sy+=trim; dy+=trim; h-=trim;
    w=min(w,min(s->w-sx,d->clip.x+d->clip.w-dx));
    h=min(h,min(s->h-sy,d->clip.y+d->clip.h-dy));
    if (w<=0 || h<=0) return 0;
    /* Text scrolling uses overlapping self-blits. */
    start=0; end=h; step=1;
    if (s==d && dy>sy) { start=h-1; end=-1; step=-1; }
    for (y=start;y!=end;y+=step) {
        Uint32 *src=(Uint32 *)s->pixels+(size_t)(sy+y)*s->w+sx;
        Uint32 *dst=(Uint32 *)d->pixels+(size_t)(dy+y)*d->w+dx;
        if (!(s->flags & SDL_SRCCOLORKEY)) memmove(dst,src,(size_t)w*4);
        else if (s==d && dy==sy && dx>sx) {
            for(x=w-1;x>=0;x--) if(src[x]!=s->key) dst[x]=src[x];
        } else {
            for(x=0;x<w;x++) if(src[x]!=s->key) dst[x]=src[x];
        }
    }
    if(dr) { dr->x=(Sint16)dx; dr->y=(Sint16)dy; dr->w=(Uint16)w; dr->h=(Uint16)h; }
    return 0;
}

static void queue_event(SDL_Event e)
{
    unsigned next=(write_event+1)%256;
    if(e.type==SDL_VIDEORESIZE && write_event!=read_event &&
       events[(write_event+255)%256].type==SDL_VIDEORESIZE) {
        events[(write_event+255)%256]=e;
    } else if(next!=read_event) { events[write_event]=e; write_event=next; }
}

static int modifiers(void)
{
    int m=0;
    if(GetKeyState(VK_SHIFT)&0x8000) m|=KMOD_SHIFT;
    if(GetKeyState(VK_CONTROL)&0x8000) m|=KMOD_CTRL;
    if(GetKeyState(VK_LMENU)&0x8000) m|=KMOD_LALT;
    if(GetKeyState(VK_RMENU)&0x8000) m|=KMOD_RALT;
    if(GetKeyState(VK_NUMLOCK)&1) m|=KMOD_NUM;
    return m;
}

static int special_key(WPARAM vk)
{
    if(vk>=VK_F1 && vk<=VK_F15) return SDLK_F1+(int)(vk-VK_F1);
    switch(vk) {
    case VK_RETURN:return SDLK_RETURN; case VK_BACK:return SDLK_BACKSPACE;
    case VK_TAB:return SDLK_TAB; case VK_ESCAPE:return SDLK_ESCAPE;
    case VK_DELETE:return SDLK_DELETE; case VK_UP:return SDLK_UP;
    case VK_DOWN:return SDLK_DOWN; case VK_LEFT:return SDLK_LEFT;
    case VK_RIGHT:return SDLK_RIGHT; case VK_HOME:return SDLK_HOME;
    case VK_END:return SDLK_END; case VK_INSERT:return SDLK_INSERT;
    case VK_PRIOR:return SDLK_PAGEUP; case VK_NEXT:return SDLK_PAGEDOWN;
    case VK_CANCEL:return SDLK_BREAK; case VK_HELP:return SDLK_HELP;
    default:return 0;
    }
}

static void paint(HDC dc)
{
    RECT client;
    BITMAPINFO bmi;
    GetClientRect(window,&client);
    if(!screen) { FillRect(dc,&client,(HBRUSH)GetStockObject(BLACK_BRUSH)); return; }
    memset(&bmi,0,sizeof(bmi));
    bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth=screen->w; bmi.bmiHeader.biHeight=-screen->h;
    bmi.bmiHeader.biPlanes=1; bmi.bmiHeader.biBitCount=32;
    bmi.bmiHeader.biCompression=BI_RGB;
    SetStretchBltMode(dc,COLORONCOLOR);
    StretchDIBits(dc,0,0,client.right,client.bottom,0,0,screen->w,screen->h,
                  screen->pixels,&bmi,DIB_RGB_COLORS,SRCCOPY);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    SDL_Event e;
    memset(&e,0,sizeof(e));
    switch(msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc=BeginPaint(hwnd,&ps);
        if(window) paint(dc);
        EndPaint(hwnd,&ps);
        return 0;
    }
    case WM_ERASEBKGND:return 1;
    case WM_CLOSE:e.type=SDL_QUIT; queue_event(e); return 0;
    case WM_SIZE:
        if(!changing_mode && wp!=SIZE_MINIMIZED && LOWORD(lp) && HIWORD(lp)) {
            e.type=SDL_VIDEORESIZE; e.resize.w=LOWORD(lp); e.resize.h=HIWORD(lp);
            queue_event(e);
        }
        return 0;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lp)->ptMinTrackSize.x=320;
        ((MINMAXINFO *)lp)->ptMinTrackSize.y=240;
        return 0;
    case WM_KEYDOWN:case WM_SYSKEYDOWN:
        if(msg==WM_SYSKEYDOWN && wp==VK_F4) break; /* Native Alt+F4 close. */
        e.key.keysym.sym=special_key(wp);
        e.key.keysym.mod=modifiers();
        if(!e.key.keysym.sym && (e.key.keysym.mod & KMOD_CTRL) &&
           !(e.key.keysym.mod & KMOD_RALT) && wp>='A' && wp<='Z')
            e.key.keysym.sym=(int)wp-'A'+'a';
        if(e.key.keysym.sym) { e.type=SDL_KEYDOWN; queue_event(e); return 0; }
        break; /* TranslateMessage supplies keyboard-layout-aware WM_CHAR. */
    case WM_CHAR:case WM_SYSCHAR:
        if(wp>=32 && wp<=126) {
            e.type=SDL_KEYDOWN; e.key.keysym.unicode=(Uint16)wp;
            e.key.keysym.sym=(wp>='A' && wp<='Z')?(int)wp-'A'+'a':(int)wp;
            e.key.keysym.mod=modifiers(); queue_event(e);
        }
        return 0;
    case WM_LBUTTONDOWN:case WM_MBUTTONDOWN:case WM_RBUTTONDOWN: {
        RECT client;
        GetClientRect(hwnd,&client);
        if(screen && client.right>0 && client.bottom>0) {
            e.type=SDL_MOUSEBUTTONDOWN;
            e.button.button=msg==WM_LBUTTONDOWN?1:msg==WM_MBUTTONDOWN?2:3;
            e.button.x=GET_X_LPARAM(lp)*screen->w/client.right;
            e.button.y=GET_Y_LPARAM(lp)*screen->h/client.bottom;
            queue_event(e);
        }
        return 0;
    }
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}

int SDL_Init(Uint32 flags)
{
    WNDCLASSW wc;
    (void)flags;
    memset(&wc,0,sizeof(wc));
    wc.lpfnWndProc=window_proc; wc.hInstance=GetModuleHandleW(NULL);
    wc.hCursor=LoadCursor(NULL,IDC_ARROW); wc.lpszClassName=window_class;
    if(!RegisterClassW(&wc) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) {
        win_error("RegisterClass"); return -1;
    }
    SetProcessDPIAware();
    return 0;
}

static void set_fullscreen(int enabled)
{
    changing_mode=1;
    if(enabled) {
        MONITORINFO mi;
        mi.cbSize=sizeof(mi);
        GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&mi);
        GetWindowRect(window,&saved_window);
        window_style=(DWORD)GetWindowLongPtrW(window,GWL_STYLE);
        SetWindowLongPtrW(window,GWL_STYLE,WS_POPUP|WS_VISIBLE);
        SetWindowPos(window,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,
                     mi.rcMonitor.right-mi.rcMonitor.left,
                     mi.rcMonitor.bottom-mi.rcMonitor.top,SWP_FRAMECHANGED);
    } else {
        SetWindowLongPtrW(window,GWL_STYLE,window_style);
        SetWindowPos(window,NULL,saved_window.left,saved_window.top,
                     saved_window.right-saved_window.left,
                     saved_window.bottom-saved_window.top,
                     SWP_FRAMECHANGED|SWP_NOZORDER);
    }
    fullscreen=enabled;
    changing_mode=0;
}

SDL_Surface *SDL_SetVideoMode(int w,int h,int bpp,Uint32 flags)
{
    SDL_Surface *s=SDL_CreateRGBSurface(flags & ~(SDL_HWSURFACE|SDL_DOUBLEBUF),w,h,bpp,0,0,0,0);
    RECT r={0,0,w,h},client;
    DWORD style=WS_OVERLAPPEDWINDOW;
    if(!s) return NULL;
    changing_mode=1;
    if(!(flags & SDL_RESIZABLE)) style &= ~(WS_THICKFRAME|WS_MAXIMIZEBOX);
    if(flags & SDL_NOFRAME) style=WS_POPUP;
    AdjustWindowRect(&r,style,FALSE);
    if(!window) {
        window=CreateWindowExW(0,window_class,L"pMARS - Win32/GDI",style,
                              CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,
                              NULL,NULL,GetModuleHandleW(NULL),NULL);
        if(!window) { win_error("CreateWindow"); SDL_FreeSurface(s); changing_mode=0; return NULL; }
    } else if(!fullscreen) {
        GetClientRect(window,&client);
        if(client.right!=w || client.bottom!=h)
            SetWindowPos(window,NULL,0,0,r.right-r.left,r.bottom-r.top,SWP_NOMOVE|SWP_NOZORDER);
    }
    screen=s;
    if(!!(flags & SDL_FULLSCREEN)!=fullscreen) set_fullscreen(!!(flags & SDL_FULLSCREEN));
    changing_mode=0;
    ShowWindow(window,SW_SHOW);
    return s;
}

const SDL_VideoInfo *SDL_GetVideoInfo(void)
{ static const SDL_VideoInfo info={0,0}; return &info; }
SDL_Rect **SDL_ListModes(SDL_PixelFormat *f,Uint32 flags)
{ (void)f; (void)flags; return (SDL_Rect **)-1; }
void SDL_WM_SetCaption(const char *title,const char *icon)
{ (void)title; (void)icon; SetWindowTextW(window,L"pMARS - Win32/GDI"); }

int SDL_WM_ToggleFullScreen(SDL_Surface *s)
{
    /* Retain logical framebuffer size, scale to the monitor. Restore exactly
     * the previous window rectangle; mouse coordinates are mapped back. */
    if(!window) return 0;
    set_fullscreen(!fullscreen);
    if(fullscreen) s->flags|=SDL_FULLSCREEN; else s->flags &= ~SDL_FULLSCREEN;
    InvalidateRect(window,NULL,FALSE);
    return 1;
}

void SDL_UpdateRect(SDL_Surface *s,int x,int y,int w,int h)
{
    (void)x; (void)y; (void)w; (void)h;
    if(s==screen && window) InvalidateRect(window,NULL,FALSE);
}
int SDL_Flip(SDL_Surface *s)
{ SDL_UpdateRect(s,0,0,0,0); if(window) UpdateWindow(window); return 0; }

int SDL_PollEvent(SDL_Event *e)
{
    MSG msg;
    while(read_event==write_event && PeekMessageW(&msg,NULL,0,0,PM_REMOVE)) {
        if(msg.message==WM_QUIT) { memset(e,0,sizeof(*e)); e->type=SDL_QUIT; return 1; }
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    /* Paint even when input is arriving continuously. */
    if(window) UpdateWindow(window);
    if(read_event==write_event) return 0;
    *e=events[read_event]; read_event=(read_event+1)%256;
    return 1;
}
int SDL_WaitEvent(SDL_Event *e)
{
    for(;;) {
        if(SDL_PollEvent(e)) return 1;
        if(MsgWaitForMultipleObjects(0,NULL,FALSE,INFINITE,QS_ALLINPUT)==WAIT_FAILED) {
            memset(e,0,sizeof(*e)); e->type=SDL_QUIT; return 1;
        }
    }
}
void SDL_WarpMouse(int x,int y)
{
    RECT r;
    POINT p;
    if(!window || !screen || GetForegroundWindow()!=window) return;
    GetClientRect(window,&r);
    p.x=x*r.right/screen->w; p.y=y*r.bottom/screen->h;
    ClientToScreen(window,&p); SetCursorPos(p.x,p.y);
}
void SDL_Delay(Uint32 ms) { Sleep(ms); }
void SDL_Quit(void)
{
    if(window) { HWND old=window; window=NULL; DestroyWindow(old); }
    fullscreen=0; read_event=write_event=0;
    UnregisterClassW(window_class,GetModuleHandleW(NULL));
}
