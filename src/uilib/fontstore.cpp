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
    FONT font = TTF_OpenFont(asset(name).c_str(), size);
    if (!font) fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
    _store[name][size] = font;
    return font;
}

} // namespace
