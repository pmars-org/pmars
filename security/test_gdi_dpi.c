/* Native DPI, resize and input regression. GPL-2.0-or-later. */
#include <assert.h>
#include "../src/gdidisp.c"

/* Choose a physical pixel inside logical pixel (160,120). Truncating
 * client_size/4 can land just BEFORE that pixel when Windows clamps a window
 * to a smaller desktop, or the client dimensions are not divisible by four. */
static void check_mouse_mapping(void)
{
    RECT client;
    SDL_Event event;
    int x, y;
    while(SDL_PollEvent(&event)) { }
    GetClientRect(window,&client);
    assert(client.right >= 640 && client.bottom >= 480);
    x=(160*client.right+639)/640;
    y=(120*client.bottom+479)/480;
    SendMessageW(window,WM_LBUTTONDOWN,0,MAKELPARAM(x,y));
    assert(SDL_PollEvent(&event) && event.type==SDL_MOUSEBUTTONDOWN);
    if(event.button.x!=160 || event.button.y!=120)
        fprintf(stderr,"client %ld x %ld, click %d,%d mapped to %d,%d\n",
                client.right,client.bottom,x,y,event.button.x,event.button.y);
    assert(event.button.x==160 && event.button.y==120);
}

int main(void)
{
    RECT client, target;
    SDL_Event event;
    SDL_Surface *surface;
    UINT dpi;
    int scales[] = {96, 120, 144, 192};
    unsigned i;
    assert(SDL_Init(0) == 0);
    surface = SDL_SetVideoMode(640,480,32,SDL_RESIZABLE);
    assert(surface);
    dpi = GetDpiForWindow(window);
    GetClientRect(window,&client);
    assert(client.right == MulDiv(640,dpi,96));
    assert(client.bottom == MulDiv(480,dpi,96));
    assert(surface->w == 640 && surface->h == 480);
    printf("Native GDI DPI: %u; client: %ld x %ld\n",dpi,client.right,client.bottom);
    for(i=0;i<sizeof(scales)/sizeof(scales[0]);++i) {
        dpi=scales[i];
        target.left=target.top=0;
        target.right=MulDiv(640,dpi,96); target.bottom=MulDiv(480,dpi,96);
        /* Use the actual frame metrics for this monitor when simulating a
         * DPI notification; the client area must match the simulated scale. */
        AdjustWindowRectExForDpi(&target,WS_OVERLAPPEDWINDOW,FALSE,0,GetDpiForWindow(window));
        OffsetRect(&target,100-target.left,100-target.top);
        SendMessageW(window,WM_DPICHANGED,MAKELONG(dpi,dpi),(LPARAM)&target);
        assert(window_dpi==dpi && screen->w==640 && screen->h==480);
        check_mouse_mapping();
        SendMessageW(window,WM_SIZE,SIZE_RESTORED,
                     MAKELPARAM(MulDiv(800,dpi,96),MulDiv(600,dpi,96)));
        assert(SDL_PollEvent(&event) && event.type==SDL_VIDEORESIZE);
        assert(event.resize.w==800 && event.resize.h==600);
    }
    /* Exercise the rounding case even on a large desktop. */
    target.left=target.top=0; target.right=641; target.bottom=481;
    AdjustWindowRectExForDpi(&target,WS_OVERLAPPEDWINDOW,FALSE,0,GetDpiForWindow(window));
    assert(SetWindowPos(window,NULL,0,0,target.right-target.left,
                        target.bottom-target.top,SWP_NOMOVE|SWP_NOZORDER));
    GetClientRect(window,&client);
    assert(client.right==641 && client.bottom==481);
    check_mouse_mapping();
    SDL_FreeSurface(surface);
    SDL_Quit();
    puts("PASS: GDI window sizing, DPI changes, logical resize and mouse at 100/125/150/200%");
    return 0;
}
