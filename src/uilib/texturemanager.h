#pragma once

#include <map>
#include <SDL2/SDL.h>

#include "widget.h"
#include "../core/fs.h"

namespace Ui {

struct SDLTextureDeleter {
    void operator()(SDL_Texture* tex) const
    {
        SDL_DestroyTexture(tex);
    }
};

class TextureManager {
    Renderer _renderer;
    std::map<fs::path, std::unique_ptr<SDL_Texture, SDLTextureDeleter>> _textures;

public:
    // TODO: move TextureManager instances inside Renderers
    static TextureManager& get(Renderer renderer);
    static SDL_Texture* get(Renderer renderer, const fs::path& path);
    static void remove(Renderer renderer);

    explicit TextureManager(Renderer renderer);

    SDL_Texture* get(const fs::path& path);
};

} // namespace Ui
