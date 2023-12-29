#include "imagefilter.h"
#include "colorhelper.h"
#include <stdio.h>

namespace Ui {

SDL_Surface* ImageFilter::apply(SDL_Surface *surf) const
{
    if (!surf) return surf;
    
    if (name=="grey" || name=="disable") {
        return makeGreyscale(surf, true);
    }
    if (name=="overlay" && !args.empty()) {
        auto overlay = IMG_Load_RW(SDL_RWFromMem((void*)args[0].c_str(), (int)args[0].length()), 1);
        overlay = makeTransparent(overlay, 0xff, 0x00, 0xff, false);
        if (overlay) {
            bool first = true;
            for (const auto& arg: args) {
                if (first) {
                    first = false; // skip the first arg (the image)
                } else {
                    overlay = ImageFilter(arg).apply(overlay);
                }
            }
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
