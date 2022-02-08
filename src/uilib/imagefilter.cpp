#include "imagefilter.h"
#include "colorhelper.h"
#include <stdio.h>

namespace Ui {

SDL_Surface* ImageFilter::apply(SDL_Surface *surf)
{
    if (!surf) return surf;
    
    if (name=="grey" || name=="disable") {
        return makeGreyscale(surf, true);
    }
    if (name=="overlay" && !arg.empty()) {
        auto overlay = IMG_Load_RW(SDL_RWFromMem((void*)arg.c_str(), (int)arg.length()), 1);
        overlay = makeTransparent(overlay, 0xff, 0x00, 0xff, false);
        if (overlay) {
            SDL_BlitSurface(overlay, nullptr, surf, nullptr);
            SDL_FreeSurface(overlay);
        }
        else {
            fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
        }        
    }
    
    return surf;
}

} // namespace
