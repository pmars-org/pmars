/* Native DPI, resize and input regression. GPL-2.0-or-later. */
#include <assert.h>
#include "../src/gdidisp.c"

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
        while(SDL_PollEvent(&event)) { }
        GetClientRect(window,&client);
        SendMessageW(window,WM_LBUTTONDOWN,0,
                     MAKELPARAM(client.right/4,client.bottom/4));
        assert(SDL_PollEvent(&event) && event.type==SDL_MOUSEBUTTONDOWN);
        assert(event.button.x==160 && event.button.y==120);
        SendMessageW(window,WM_SIZE,SIZE_RESTORED,
                     MAKELPARAM(MulDiv(800,dpi,96),MulDiv(600,dpi,96)));
        assert(SDL_PollEvent(&event) && event.type==SDL_VIDEORESIZE);
        assert(event.resize.w==800 && event.resize.h==600);
    }
    SDL_FreeSurface(surface);
    SDL_Quit();
    puts("PASS: GDI window sizing, DPI changes, logical resize and mouse at 100/125/150/200%");
    return 0;
}
