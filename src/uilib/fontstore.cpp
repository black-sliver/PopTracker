#include "fontstore.h"
#include "../core/assets.h"


namespace Ui {

FontStore::FontStore()
{
    if (TTF_Init() != 0) {
        fprintf(stderr, "Error initializing SDL_TTF: %s\n", TTF_GetError());
        throw std::runtime_error(TTF_GetError());
    }
}

FontStore::~FontStore()
{
    for (const auto& namePair: _store) {
        for (const auto& sizePair: namePair.second) {
            TTF_CloseFont(sizePair.second);
        }
    }
    TTF_Quit();
}

FontStore::FONT FontStore::getFont(const char* name, int size)
{
    const auto nameIt = _store.find(name);
    if (nameIt != _store.end()) {
        const auto sizeIt = nameIt->second.find(size);
        if (sizeIt != nameIt->second.end()) {
            return sizeIt->second;
        }
    }

#ifdef _WIN32
    // on Windows, SDL_ttf uses SDL's IO functions, which expect UTF8
    FONT font = TTF_OpenFont(asset(name).u8string().c_str(), size);
#else
    // otherwise it's probably fopen, which is "native" encoding
    FONT font = TTF_OpenFont(asset(name).c_str(), size);
#endif
    if (!font) {
        fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
    }
    _store[name][size] = font;
    return font;
}

} // namespace
