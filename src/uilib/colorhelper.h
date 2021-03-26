#ifndef _UILIB_COLORHELPER_H
#define _UILIB_COLORHELPER_H

#include <stdint.h>
#include <SDL2/SDL_image.h>


#define BW_LUMINOSITY // modes for greyscale are BW_LUMINOSITY, BW_LIGHTNESS and BW_AVERAGE
#define BW_DARKEN // makes the greyscale darker if defined



#if defined BW_LUMINOSITY
static uint8_t makeGreyscale(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t v = 0;
    v += (((uint16_t)r)*21);
    v += (((uint16_t)g)*72);
    v += (((uint16_t)b)* 7);
    #ifdef BW_DARKEN
    v *= 2; v += 1; v /= 3;
    #endif
    return (uint8_t)((v+50)/100);
}
#elif defined BW_LIGHTNESS
#define MAX3(a,b,c) ( ((a)>=(b) && (a)>=(c)) ? (a) : \
                      ((b)>=(a) && (b)>=(c)) ? (b) : (c) )
#define MIN3(a,b,c) ( ((a)<=(b) && (a)<=(c)) ? (a) : \
                      ((b)<=(a) && (b)<=(c)) ? (b) : (c) )
static uint8_t makeGreyscale(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t v = MAX3(r, g, b);
    v += MIN3(r, g, b);
    #ifdef BW_DARKEN
    v *= 2; v += 1; v /= 3;
    #endif
    return (uint8_t)((v+1)/2);
}
#else // BW_AVERAGE
static uint8_t makeGreyscale(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t v = r; v += g; v += b;
    #ifdef BW_DARKEN
    v *= 2; v += 1; v /= 3;
    #endif
    return (uint8_t)((2*v+3)/6);
}
#endif
static SDL_Color makeGreyscale(SDL_Color c)
{
    uint8_t w = makeGreyscale(c.r,c.g,c.b);
    return { w, w, w, c.a };
}

static SDL_Surface *makeGreyscale(SDL_Surface *surf)
{
    SDL_LockSurface(surf);
    if (surf->format->BitsPerPixel <= 8) { // convert palette to greyscale
        int ncolors = surf->format->palette->ncolors;
        SDL_Palette* pal = SDL_AllocPalette(ncolors);
        for (int i=0; i<ncolors; i++) {
            pal->colors[i] = makeGreyscale(surf->format->palette->colors[i]);
        }
        SDL_UnlockSurface(surf);
        
        SDL_SetSurfacePalette(surf, pal);
        SDL_FreePalette(pal); // FIXME: is this required / allowed?
    }
    else {
        uint8_t *data = (uint8_t*)surf->pixels;
        auto fmt = surf->format;
        for (int y=0; y<surf->h; y++) {
            for (int x=0; x<surf->w; x++) {
                uint32_t* ppixel = (uint32_t*)(data+y*surf->pitch+x*fmt->BytesPerPixel);
                uint32_t pixel = *ppixel;
                uint8_t w = makeGreyscale(
                    ((pixel & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss,
                    ((pixel & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss,
                    ((pixel & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss);
                pixel &= ~(fmt->Rmask|fmt->Gmask|fmt->Bmask);
                pixel |= (w >> fmt->Rloss) << fmt->Rshift;
                pixel |= (w >> fmt->Gloss) << fmt->Gshift;
                pixel |= (w >> fmt->Bloss) << fmt->Bshift;
                memcpy(ppixel, &pixel, fmt->BytesPerPixel);
            }
        }
        SDL_UnlockSurface(surf);
    }
    return surf;
}


#endif // _UILIB_COLORHELPER_H
