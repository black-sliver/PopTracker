#ifndef _UILIB_IMAGEFILTER_H
#define _UILIB_IMAGEFILTER_H


#include "colorhelper.h"
#include <string>
#include <vector>
#include <SDL2/SDL_image.h>


namespace Ui {

struct ImageFilter {
    ImageFilter(std::string name)
        : name(name)
    {
    }

    ImageFilter(std::string name, std::string arg)
        : name(name), args({arg})
    {
    }

    ImageFilter(std::string name, std::vector<std::string>& args)
        : name(name), args(args)
    {
    }

    std::string name;
    std::vector<std::string> args;
    
    SDL_Surface* apply(SDL_Surface *surf) const;

    bool operator==(const ImageFilter& rhs) const
    {
        return name == rhs.name && args == rhs.args;
    }
};

} // namespace Ui

#endif // _UILIB_IMAGEFILTER_H
