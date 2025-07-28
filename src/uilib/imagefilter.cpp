#include "imagefilter.h"
#include <cstdio>
#include <cstdlib>
#include "colorhelper.h"

namespace Ui {

static float tryParseFloat(const std::string& s, const float defaultVal, const float minVal, const float maxVal)
{
    char* end = nullptr;
    float res = std::strtof(s.c_str(), &end);
    if (std::isnan(res) || !end || end == s.c_str() || end != s.c_str() + s.length()) {
        // invalid or empty argument
        res = defaultVal;
    } else if (res < minVal) {
        res = minVal;
    } else if (res > maxVal) {
        res = maxVal;
    }
    return res;
}

SDL_Surface* ImageFilter::apply(SDL_Surface *surf) const
{
    if (!surf) return surf;
    
    if (name == "grey" || name == "disable") {
        return makeGreyscale(surf, true);
    }
    if (name == "saturation") {
        // arg1: fraction (0..1); default 0.5
        // arg2: colorspace; bt601 = "color-corrected"; all other: AVG(r, g, b)
        float value = 0.5;
        bool mode = false;
        if (!args.empty()) {
            value = tryParseFloat(args[0], 0.5f, 0.f, 255.f);
            if (args.size() > 1)
                mode = strcasecmp(args[1].c_str(), "bt601") == 0 || strcasecmp(args[1].c_str(), "bt.601") == 0;
        }
        if (mode)
            return setSaturation<BWMode::Luminosity>(surf, value);
        return setSaturation<BWMode::Average>(surf, value);
    }
    if (name == "brightness") {
        // arg1: fraction (0..1); default 0.5
        // arg2: colorspace; currently all handled the same
        float value = 0.5;
        if (!args.empty()) {
            value = tryParseFloat(args[0], 0.5f, 0.f, 255.f);
        }
        return setBrightness(surf, value);
    }
    if (name == "greyscale") {
        // same as saturation|0
        // arg1: colorspace; bt601 = "color-corrected"; all other: AVG(r, g, b)
        bool mode = false;
        if (!args.empty())
            mode = strcasecmp(args[0].c_str(), "bt601") == 0 || strcasecmp(args[0].c_str(), "bt.601") == 0;

        if (mode)
            return makeGreyscale<BWMode::Luminosity>(surf, false);
        return makeGreyscale<BWMode::Average>(surf, false);
    }
    if (name == "dim") {
        // same as brightness|0.5
        // arg1: colorspace; currently all handled the same
        return setBrightness(surf, 0.5);
    }
    if (name == "overlay" && !args.empty()) {
        auto overlay = IMG_Load_RW(SDL_RWFromMem((void*)args[0].c_str(), (int)args[0].length()), 1);
        if (overlay) {
            overlay = makeTransparent(overlay, 0xff, 0x00, 0xff, false);
        }
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
