#ifndef _UILIB_COLORHELPER_H
#define _UILIB_COLORHELPER_H

#include <stdint.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>


#define BW_LUMINOSITY // modes for greyscale are BW_LUMINOSITY, BW_LIGHTNESS and BW_AVERAGE


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
static inline __attribute__((always_inline))
uint8_t makeGreyscale(uint8_t r, uint8_t g, uint8_t b, bool darken)
{
    uint16_t v = 0;
    v += (((uint16_t)r)*21);
    v += (((uint16_t)g)*72);
    v += (((uint16_t)b)* 7);
    if (darken) {
        v *= 2; v += 1; v /= 3;
    }
    return (uint8_t)((v+50)/100);
}
#elif defined BW_LIGHTNESS
#define MAX3(a,b,c) ( ((a)>=(b) && (a)>=(c)) ? (a) : \
                      ((b)>=(a) && (b)>=(c)) ? (b) : (c) )
#define MIN3(a,b,c) ( ((a)<=(b) && (a)<=(c)) ? (a) : \
                      ((b)<=(a) && (b)<=(c)) ? (b) : (c) )
static inline __attribute__((always_inline))
uint8_t makeGreyscale(uint8_t r, uint8_t g, uint8_t b, bool darken)
{
    uint16_t v = MAX3(r, g, b);
    v += MIN3(r, g, b);
    if (darken) {
        v *= 2; v += 1; v /= 3;
    }
    return (uint8_t)((v+1)/2);
}
#else // BW_AVERAGE
static inline __attribute__((always_inline))
uint8_t makeGreyscale(uint8_t r, uint8_t g, uint8_t b, bool darken)
{
    uint16_t v = r; v += g; v += b;
    if (darken) {
        v *= 2; v += 1; v /= 3;
    }
    return (uint8_t)((2*v+3)/6);
}
#endif

static SDL_Color makeGreyscale(SDL_Color c, bool darken)
{
    uint8_t w = makeGreyscale(c.r,c.g,c.b, darken);
    return { w, w, w, c.a };
}

static inline __attribute__((always_inline))
uint32_t bytesToUint(const uint8_t* b, const uint8_t bytespp)
{
    // this is basically like memcpy, but there is no actual 24bit uint
    uint32_t res = 0;
    for (uint8_t i=0; i<bytespp; i++) {
        res <<= 8;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN // [0] = msb
        res |= b[i];
#else // [0] = lsb
        res |= b[bytespp-i-1];
#endif
    }
    return res;
}

static inline __attribute__((always_inline))
void uintToBytes(uint32_t d, uint8_t* b, const uint8_t bytespp)
{
    // this is basically like memcpy, but there is no actual 24bit uint
    for (uint8_t i=0; i<bytespp; i++) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN // [0] = msb
        b[bytespp - i - 1] = (uint8_t)(d&0xff);
#else // [0] = lsb
        b[i] = (uint8_t)(d&0xff);
#endif
        d >>= 8;
    }
}

static inline __attribute__((always_inline))
void _makeGreyscaleRGB(SDL_Surface *surf, bool darken)
{
    const uint8_t bytespp = surf->format->BytesPerPixel;
    uint8_t* data = (uint8_t*)surf->pixels;
    auto fmt = surf->format;
    for (int y=0; y<surf->h; y++) {
        for (int x=0; x<surf->w; x++) {
            uint8_t* ppixel = data + y * surf->pitch + x * bytespp;
            uint32_t pixel = bytesToUint(ppixel, bytespp);
            uint8_t w = makeGreyscale(
                ((pixel & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss,
                ((pixel & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss,
                ((pixel & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss, darken);
            pixel &= ~(fmt->Rmask|fmt->Gmask|fmt->Bmask);
            pixel |= (w >> fmt->Rloss) << fmt->Rshift;
            pixel |= (w >> fmt->Gloss) << fmt->Gshift;
            pixel |= (w >> fmt->Bloss) << fmt->Bshift;
            uintToBytes(pixel, ppixel, bytespp);
        }
    }
}

static inline __attribute__((always_inline))
SDL_Surface *makeGreyscale(SDL_Surface *surf, bool darken)
{
    // inline since it should only be used in a few places
    // by inlining all of it, we get rid of 'darken' during runtime regardless of optimization level,
    // but profiling output (gprof) will be less readable
    if (SDL_MUSTLOCK(surf)) {
        if (SDL_LockSurface(surf) != 0) {
            fprintf(stderr, "makeGreyscale: Could not lock surface: %s\n",
                    SDL_GetError());
            return surf;
        }
    }
    if (surf->format->BitsPerPixel <= 8 && surf->format->palette) {
        // convert palette to greyscale
        int ncolors = surf->format->palette->ncolors;
        SDL_Palette* pal = SDL_AllocPalette(ncolors);
        for (int i=0; i<ncolors; i++) {
            pal->colors[i] = makeGreyscale(surf->format->palette->colors[i], darken);
        }
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);

        SDL_SetSurfacePalette(surf, pal); // refcount++
        SDL_FreePalette(pal); // refcount--, will not actually free it here
    }
    else if (surf->format->palette) {
        fprintf(stderr, "makeGreyscale: Unsupported palette: %hhubit\n",
                surf->format->BitsPerPixel);
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);
    }
    else if (surf->format->BytesPerPixel > 0 && surf->format->BytesPerPixel < 5) {
        _makeGreyscaleRGB(surf, darken);
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);
    }
    else {
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);
        fprintf(stderr, "makeGreyscale: Unsupported pixel format\n");
    }
    return surf;
}

static SDL_Surface *makeTransparent(SDL_Surface *surf, uint8_t r, uint8_t g, uint8_t b, bool allowColorKey)
{
    // if any corner pixel is [r,g,b], make that color transparent
    bool doColorKey = false;
    if (!SDL_MUSTLOCK(surf) || SDL_LockSurface(surf) == 0) {
        for (uint8_t n=0; !doColorKey && n<4; n++) {
            uint8_t r1,g1,b1,a1;
            uint32_t px = getPixel(surf, (n&1) ? surf->w-1 : 0, (n&2) ? surf-> h-1 : 0);
            SDL_GetRGBA(px, surf->format, &r1, &g1, &b1, &a1);
            doColorKey = (r1==r && g1==g && b1==b && a1==0xff);
        }
        if (SDL_MUSTLOCK(surf))
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
            if (!SDL_MUSTLOCK(surf) || SDL_LockSurface(surf) == 0) {
                uint32_t key = SDL_MapRGB(surf->format, r, g, b);
                for (int y=0; y<surf->h; y++) {
                    uint32_t* vals = (uint32_t*)((uint8_t*)surf->pixels + y*surf->pitch);
                    for (int x=0; x<surf->w; x++) {
                        if (vals[x] == key) vals[x] = 0;
                    }
                }
                if (SDL_MUSTLOCK(surf))
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
