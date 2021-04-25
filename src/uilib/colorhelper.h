#ifndef _UILIB_COLORHELPER_H
#define _UILIB_COLORHELPER_H

#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>


#define BW_LUMINOSITY // modes for greyscale are BW_LUMINOSITY, BW_LIGHTNESS and BW_AVERAGE
#define BW_DARKEN // makes the greyscale darker if defined


static Uint32 getPixel(SDL_Surface* surf, unsigned x, unsigned y)
{
    unsigned Bpp = surf->format->BytesPerPixel;
    Uint8* p = (Uint8*)surf->pixels + y * surf->pitch + x * Bpp;
    switch (Bpp) {
        case 1:
            return *p;
        case 2:
            return *(Uint16*)p;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
        case 4:
            return *(Uint32*)p;
        default:
            return 0;
    }
}


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
    if (SDL_LockSurface(surf) != 0) {
        fprintf(stderr, "Could not lock surface to make greyscale: %s\n",
                SDL_GetError());
        return surf;
    }
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

static SDL_Surface *makeTransparent(SDL_Surface *surf, uint8_t r, uint8_t g, uint8_t b, bool allowColorKey)
{
    // if any corner pixel is [r,g,b], make that color transparent
    bool doColorKey = false;
    if (SDL_LockSurface(surf) == 0) {
        for (uint8_t n=0; !doColorKey && n<4; n++) {
            uint8_t r1,g1,b1,a1;
            uint32_t px = getPixel(surf, (n&1) ? surf->w-1 : 0, (n&2) ? surf-> h-1 : 0);
            SDL_GetRGBA(px, surf->format, &r1, &g1, &b1, &a1);
            doColorKey = (r1==r && g1==g && b1==b && a1==0xff);
        }
        SDL_UnlockSurface(surf);
    } else {
        fprintf(stderr, "Could not lock surface to check corners: %s\n",
                SDL_GetError());
    }
    if (doColorKey) {
        if (allowColorKey) {
            // we can just set a color key
            SDL_SetColorKey(surf, SDL_TRUE, SDL_MapRGB(surf->format, r, g, b));
        } else {
            // we need to modify the surface
            if (!surf->format->Amask || surf->format->BytesPerPixel!=4) {
                // convert to have alpha
                auto newFmt = SDL_AllocFormat(SDL_PIXELFORMAT_ARGB8888);
                auto newSurf = SDL_ConvertSurface(surf, newFmt, 0);
                SDL_FreeFormat(newFmt);
                if (!newSurf) {
                    fprintf(stderr, "Could not create surface: %s\n",
                            SDL_GetError());
                    return surf;
                }
                SDL_FreeSurface(surf);
                surf = newSurf;
            }
            SDL_SetSurfaceBlendMode(surf, SDL_BLENDMODE_BLEND);
            if (SDL_LockSurface(surf) == 0) {
                uint32_t key = SDL_MapRGB(surf->format, r, g, b);
                for (int y=0; y<surf->h; y++) {
                    uint32_t* vals = (uint32_t*)((uint8_t*)surf->pixels + y*surf->pitch);
                    for (int x=0; x<surf->w; x++) {
                        if (vals[x] == key) vals[x] = 0;
                    }
                }
                SDL_UnlockSurface(surf);
            } else {
                fprintf(stderr, "Could not lock surface to make transparent: %s\n",
                        SDL_GetError());
            }
        }
    }
    return surf;
}

#endif // _UILIB_COLORHELPER_H
