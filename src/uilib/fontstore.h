#pragma once

#include <map>
#include <string>
#include <SDL2/SDL_ttf.h>


// TODO: do ref counting for fonts (and store)?


namespace Ui {

class FontStore final
{
public:
    using FONT = TTF_Font*;
    FONT getFont(const char* name, int size);
    FontStore();
    ~FontStore();
    
    static int sizeFromData(int dflt, int val) {
        // helper to get font size from default + json
        return (val<=0) ? (dflt+val) : val;
    }
protected:
    std::map<std::string, std::map<int, FONT>> _store;
};

} // namespace Ui
