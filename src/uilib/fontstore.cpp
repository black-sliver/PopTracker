#include "fontstore.h"
#include "../core/assets.h"


namespace Ui {

FontStore::FontStore() {}

FontStore::~FontStore()
{
    for (const auto& namepair: _store) {
        for (const auto& sizepair: namepair.second) {
            TTF_CloseFont(sizepair.second);
        }
    }
}

FontStore::FONT FontStore::getFont(const char* name, int size)
{
    auto nameIt = _store.find(name);
    if (nameIt == _store.end()) {
        goto create_font;
    } else {
        auto sizeIt = nameIt->second.find(size);
        if (sizeIt == nameIt->second.end()) {
            goto create_font;
        }
        return sizeIt->second;
    }
create_font:
#ifdef _WIN32
    // on Windows, SDL_ttf uses SDL's IO functions, which expect UTF8
    FONT font = TTF_OpenFont(asset(name).u8string().c_str(), size);
#else
    // otherwise it's probably fopen, which is "native" encoding
    FONT font = TTF_OpenFont(asset(name).c_str(), size);
#endif
    if (!font) fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
    _store[name][size] = font;
    return font;
}

} // namespace
