#ifndef _UILIB_IMAGEFILTER_H
#define _UILIB_IMAGEFILTER_H


#include "colorhelper.h"
#include <string>
#include <SDL2/SDL_image.h>


namespace Ui {

struct ImageFilter {
    ImageFilter(std::string name) { this->name=name; }
    ImageFilter(std::string name, std::string arg) { this->name=name; this->arg=arg; }
    
    std::string name;
    std::string arg;
    
    SDL_Surface* apply(SDL_Surface *surf);
};

} // namespace Ui

#endif // _UILIB_IMAGEFILTER_H
