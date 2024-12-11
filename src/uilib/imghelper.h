#pragma once

#include "../core/fs.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace Ui {

inline SDL_Surface* LoadImage(const fs::path& path)
{
#ifdef _WIN32
    // on Windows, SDL_Image uses SDL's IO functions, which expect UTF8
    return IMG_Load(path.u8string().c_str());
#else
    // otherwise it's probably fopen, which is "native" encoding
    return IMG_Load(path.c_str());
#endif
}

} // namespace Ui
