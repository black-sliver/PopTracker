#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <SDL2/SDL.h>


#define UI_COLORHELPER_HAS_BRIGHTNESS
#define UI_COLORHELPER_HAS_SATURATION

#define UI_COLORHELPER_GREY_MODE Luminosity // sets mode for default "grey"


namespace Ui {

enum class BWMode {
    Luminosity,
    Lightness,
    Average,
};

static Uint32 getPixel(const SDL_Surface* surf, const unsigned x, const unsigned y)
{
    const unsigned Bpp = surf->format->BytesPerPixel;
    Uint8* p = static_cast<Uint8 *>(surf->pixels) + y * surf->pitch + x * Bpp;
    switch (Bpp) {
        case 1:
            return *p;
        case 2:
            return *reinterpret_cast<Uint16 *>(p);
        case 3:
            if constexpr (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
        case 4:
            return *reinterpret_cast<Uint32 *>(p);
        default:
            return 0;
    }
}

static inline __attribute__((always_inline))
uint32_t bytesToUint(const uint8_t* b, const uint8_t bytesPP)
{
    // this is basically like memcpy, but there is no actual 24bit uint
    uint32_t res = 0;
    for (uint8_t i = 0; i < bytesPP; i++) {
        res <<= 8;
        if constexpr (SDL_BYTEORDER == SDL_BIG_ENDIAN) // [0] = msb
            res |= b[i];
        else // [0] = lsb
            res |= b[bytesPP - i - 1];
    }
    return res;
}

static inline __attribute__((always_inline))
void uintToBytes(uint32_t d, uint8_t* b, const uint8_t bytesPP)
{
    // this is basically like memcpy, but there is no actual 24bit uint
    for (uint8_t i = 0; i < bytesPP; i++) {
        if constexpr (SDL_BYTEORDER == SDL_BIG_ENDIAN) // [0] = msb
            b[bytesPP - i - 1] = static_cast<uint8_t>(d & 0xff);
        else // [0] = lsb
            b[i] = static_cast<uint8_t>(d & 0xff);
        d >>= 8;
    }
}

#ifndef MAX3
#define MAX3(a,b,c) ( ((a)>=(b) && (a)>=(c)) ? (a) : \
((b)>=(a) && (b)>=(c)) ? (b) : (c) )
#endif
#ifndef MIN3
#define MIN3(a,b,c) ( ((a)<=(b) && (a)<=(c)) ? (a) : \
((b)<=(a) && (b)<=(c)) ? (b) : (c) )
#endif

template<BWMode mode, bool darken = false>
static inline __attribute__((always_inline))
uint8_t makeGreyscale(uint8_t r, uint8_t g, uint8_t b)
{
    // TODO: check if this uses SIMD, rework if not?
    if constexpr (mode == BWMode::Luminosity) {
        uint16_t v = 0;
        v += (static_cast<uint16_t>(r) * 21);
        v += (static_cast<uint16_t>(g) * 72);
        v += (static_cast<uint16_t>(b) *  7);
        if constexpr (darken) {
            v *= 2; v += 1; v /= 3;
        }
        return static_cast<uint8_t>((v + 50) / 100);
    }
    if constexpr (mode == BWMode::Lightness) {
        uint16_t v = MAX3(r, g, b);
        v += MIN3(r, g, b);
        if constexpr (darken) {
            v *= 2; v += 1; v /= 3;
        }
        return static_cast<uint8_t>((v + 1) / 2);
    }
    else { // BWMode::Average
        uint16_t v = r; v += g; v += b;
        if constexpr (darken) {
            v *= 2; v += 1; v /= 3;
        }
        return static_cast<uint8_t>((2 * v + 3) / 6);
    }
}

static inline __attribute__((always_inline))
void setBrightness(uint8_t& r, uint8_t& g, uint8_t& b, const float brightness)
{
    // TODO: properly implement brightness > 1
    r = static_cast<uint8_t>(std::min(255.f, static_cast<float>(r) * brightness));
    g = static_cast<uint8_t>(std::min(255.f, static_cast<float>(g) * brightness));
    b = static_cast<uint8_t>(std::min(255.f, static_cast<float>(b) * brightness));
}

template<BWMode mode, bool darken = false>
static inline __attribute__((always_inline))
SDL_Color setSaturation(SDL_Color c, const float saturation)
{
    const uint8_t w = makeGreyscale<mode, darken>(c.r, c.g, c.b);
    if (saturation == 0.f) {
        return { w, w, w, c.a };
    }
    const auto fw = static_cast<float>(w);
    const auto r = static_cast<uint8_t>(static_cast<float>(c.r) * saturation + fw * (1.f - saturation));
    const auto g = static_cast<uint8_t>(static_cast<float>(c.g) * saturation + fw * (1.f - saturation));
    const auto b = static_cast<uint8_t>(static_cast<float>(c.b) * saturation + fw * (1.f - saturation));
    return { r, g, b, c.a };
}

static inline __attribute__((always_inline))
SDL_Color setBrightness(SDL_Color c, const float brightness)
{
    setBrightness(c.r, c.g, c.b, brightness);
    return c;
}

template<BWMode mode, bool darken = false>
static inline __attribute__((always_inline))
void _setSaturationRGB(SDL_Surface *surf, const float saturation)
{
    assert(surf && "surf argument can't be NULL");
    const uint8_t bytesPP = surf->format->BytesPerPixel;
    auto* data = static_cast<uint8_t *>(surf->pixels);
    const auto fmt = surf->format;
    for (int y=0; y<surf->h; y++) {
        for (int x=0; x<surf->w; x++) {
            uint8_t* pPixel = data + y * surf->pitch + x * bytesPP;
            uint32_t pixel = bytesToUint(pPixel, bytesPP);
            uint8_t r = ((pixel & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss;
            uint8_t g = ((pixel & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss;
            uint8_t b = ((pixel & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss;
            const uint8_t w = makeGreyscale<mode, darken>(r, g, b);
            pixel &= ~(fmt->Rmask|fmt->Gmask|fmt->Bmask);
            if (saturation == 0.f) {
                pixel |= (w >> fmt->Rloss) << fmt->Rshift;
                pixel |= (w >> fmt->Gloss) << fmt->Gshift;
                pixel |= (w >> fmt->Bloss) << fmt->Bshift;
            } else {
                const auto fw = static_cast<float>(w);
                r = static_cast<uint8_t>(static_cast<float>(r) * saturation + fw * (1.f - saturation));
                g = static_cast<uint8_t>(static_cast<float>(g) * saturation + fw * (1.f - saturation));
                b = static_cast<uint8_t>(static_cast<float>(b) * saturation + fw * (1.f - saturation));
                pixel |= (r >> fmt->Rloss) << fmt->Rshift;
                pixel |= (g >> fmt->Gloss) << fmt->Gshift;
                pixel |= (b >> fmt->Bloss) << fmt->Bshift;
            }
            uintToBytes(pixel, pPixel, bytesPP);
        }
    }
}

static inline __attribute__((always_inline))
void _setBrightnessRGB(SDL_Surface *surf, const float brightness)
{
    assert(surf && "surf argument can't be NULL");
    const uint8_t bytesPP = surf->format->BytesPerPixel;
    auto* data = static_cast<uint8_t *>(surf->pixels);
    const auto fmt = surf->format;
    for (int y=0; y<surf->h; y++) {
        for (int x=0; x<surf->w; x++) {
            uint8_t* pPixel = data + y * surf->pitch + x * bytesPP;
            uint32_t pixel = bytesToUint(pPixel, bytesPP);
            uint8_t r = ((pixel & fmt->Rmask) >> fmt->Rshift) << fmt->Rloss;
            uint8_t g = ((pixel & fmt->Gmask) >> fmt->Gshift) << fmt->Gloss;
            uint8_t b = ((pixel & fmt->Bmask) >> fmt->Bshift) << fmt->Bloss;
            setBrightness(r, g, b, brightness);
            pixel &= ~(fmt->Rmask|fmt->Gmask|fmt->Bmask);
            pixel |= (r >> fmt->Rloss) << fmt->Rshift;
            pixel |= (g >> fmt->Gloss) << fmt->Gshift;
            pixel |= (b >> fmt->Bloss) << fmt->Bshift;
            uintToBytes(pixel, pPixel, bytesPP);
        }
    }
}

template<BWMode mode, bool darken = false>
static inline __attribute__((always_inline))
SDL_Surface *setSaturation(SDL_Surface *surf, const float saturation)
{
    // inline since it should only be used in a few places
    // by inlining all of it, we get rid of 'darken' during runtime regardless of optimization level,
    // but profiling output (gprof) will be less readable
    assert(surf && "surf argument can't be NULL");
    if (SDL_MUSTLOCK(surf)) {
        if (SDL_LockSurface(surf) != 0) {
            fprintf(stderr, "setSaturation: Could not lock surface: %s\n",
                    SDL_GetError());
            return surf;
        }
    }
    if (surf->format->BitsPerPixel <= 8 && surf->format->palette) {
        // convert palette to greyscale
        const int nColors = surf->format->palette->ncolors;
        SDL_Palette* pal = SDL_AllocPalette(nColors);
        for (int i = 0; i < nColors; i++) {
            pal->colors[i] = setSaturation<mode, darken>(surf->format->palette->colors[i], saturation);
        }
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);

        SDL_SetSurfacePalette(surf, pal); // refcount++
        SDL_FreePalette(pal); // refcount--, will not actually free it here
    }
    else if (surf->format->palette) {
        fprintf(stderr, "setSaturation: Unsupported palette: %hhu bit\n",
                surf->format->BitsPerPixel);
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);
    }
    else if (surf->format->BytesPerPixel > 0 && surf->format->BytesPerPixel < 5) {
        _setSaturationRGB<mode, darken>(surf, saturation);
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);
    }
    else {
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);
        fprintf(stderr, "setSaturation: Unsupported pixel format\n");
    }
    return surf;
}

template<BWMode mode = BWMode::UI_COLORHELPER_GREY_MODE>
static inline __attribute__((always_inline))
SDL_Surface *makeGreyscale(SDL_Surface *surf, const bool darken)
{
    if (darken) {
        return setSaturation<mode, true>(surf, 0.0f);
    }
    return setSaturation<mode, false>(surf, 0.0f);
}

static inline __attribute__((always_inline))
SDL_Surface *setBrightness(SDL_Surface *surf, const float brightness)
{
    // inline since it should only be used in a few places
    assert(surf && "surf argument can't be NULL");
    if (SDL_MUSTLOCK(surf)) {
        if (SDL_LockSurface(surf) != 0) {
            fprintf(stderr, "setBrightness: Could not lock surface: %s\n",
                    SDL_GetError());
            return surf;
        }
    }
    if (surf->format->BitsPerPixel <= 8 && surf->format->palette) {
        // convert palette to greyscale
        const int nColors = surf->format->palette->ncolors;
        SDL_Palette* pal = SDL_AllocPalette(nColors);
        for (int i = 0; i < nColors; i++) {
            pal->colors[i] = setBrightness(surf->format->palette->colors[i], brightness);
        }
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);

        SDL_SetSurfacePalette(surf, pal); // refcount++
        SDL_FreePalette(pal); // refcount--, will not actually free it here
    }
    else if (surf->format->palette) {
        fprintf(stderr, "setBrightness: Unsupported palette: %hhu bit\n",
                surf->format->BitsPerPixel);
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);
    }
    else if (surf->format->BytesPerPixel > 0 && surf->format->BytesPerPixel < 5) {
        _setBrightnessRGB(surf, brightness);
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);
    }
    else {
        if (SDL_MUSTLOCK(surf))
            SDL_UnlockSurface(surf);
        fprintf(stderr, "setBrightness: Unsupported pixel format\n");
    }
    return surf;
}

static SDL_Surface *makeTransparent(SDL_Surface *surf, uint8_t r, uint8_t g, uint8_t b, bool allowColorKey)
{
    // if any corner pixel is [r,g,b], make that color transparent
    assert(surf && "surf argument can't be NULL");
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
                    auto* vals = reinterpret_cast<uint32_t *>(static_cast<uint8_t *>(surf->pixels) + y * surf->pitch);
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

} // namespace Ui
