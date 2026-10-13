/* Regression checks for clipped blits, transparency and overlapping scrolling.
 * GPL-2.0-or-later. This test does not create a window. */
#include <assert.h>
#include <stdio.h>
#include "../src/gdidisp.h"

int main(void)
{
    SDL_Surface *s=SDL_CreateRGBSurface(0,4,4,32,0,0,0,0), *copy;
    SDL_Rect clip={1,1,2,2}, src={0,0,4,3}, dst={0,1,0,0};
    Uint32 *p;
    int i;
    assert(s);
    p=s->pixels;
    assert(SDL_MapRGB(s->format,0x12,0x34,0x56)==0x123456);
    SDL_SetClipRect(s,&clip);
    SDL_FillRect(s,NULL,7);
    for(i=0;i<16;i++) assert(p[i]==((i==5||i==6||i==9||i==10)?7:0));
    SDL_SetClipRect(s,NULL);
    for(i=0;i<16;i++) p[i]=(Uint32)i;
    SDL_BlitSurface(s,&src,s,&dst);
    for(i=4;i<16;i++) assert(p[i]==(Uint32)(i-4));
    src.y=1; dst.y=0;
    SDL_BlitSurface(s,&src,s,&dst);
    for(i=0;i<12;i++) assert(p[i]==(Uint32)i);
    copy=SDL_DisplayFormat(s);
    assert(copy && copy->pixels!=s->pixels);
    SDL_FillRect(s,NULL,99);
    SDL_SetColorKey(copy,SDL_SRCCOLORKEY,5);
    dst.x=dst.y=0;
    SDL_BlitSurface(copy,NULL,s,&dst);
    assert(p[5]==99 && p[6]==6);
    SDL_FillRect(s,NULL,99);
    dst.x=dst.y=-2;
    SDL_BlitSurface(copy,NULL,s,&dst);
    assert(p[0]==10 && p[1]==11 && p[2]==99 && p[8]==99);
    clip.x=100; clip.y=100;
    assert(!SDL_SetClipRect(s,&clip));
    SDL_FillRect(s,NULL,17);
    assert(p[0]==10);
    assert(!SDL_CreateRGBSurface(0,0,2,32,0,0,0,0));
    assert(!SDL_CreateRGBSurface(0,32768,2,32,0,0,0,0));
    SDL_FreeSurface(copy); SDL_FreeSurface(s); SDL_FreeSurface(NULL);
    puts("PASS: GDI clipping, scrolling, color key, copy and allocation checks");
    return 0;
}
