#include "texturemanager.h"
#include "imghelper.h"
#include "../core/util.h"

namespace Ui {

static std::map<Renderer, TextureManager>& getInstances()
{
    static std::map<Renderer, TextureManager> instances;
    return instances;
}

TextureManager& TextureManager::get(Renderer renderer)
{
    auto [it, _] = getInstances().try_emplace(renderer, renderer);
    return it->second;
}

SDL_Texture* TextureManager::get(Renderer renderer, const fs::path &path)
{
    return get(renderer).get(path);
}

void TextureManager::remove(const Renderer renderer)
{
    getInstances().erase(renderer);
}

TextureManager::TextureManager(Renderer renderer)
{
    _renderer = renderer;
}

SDL_Texture *TextureManager::get(const fs::path &path)
{
    const auto it = _textures.find(path);
    if (it != _textures.end()) {
        return it->second.get();
    }

    SDL_Surface* surf = LoadImage(path);
    if (!surf) {
        fprintf(stderr, "Failed to load image \"%s\": %s\n", sanitize_print(path).c_str(), SDL_GetError());
    }
    SDL_Texture* tex = surf ? SDL_CreateTextureFromSurface(_renderer, surf) : nullptr;
    if (!tex) {
        fprintf(stderr, "Failed to create texture for \"%s\": %s\n", sanitize_print(path).c_str(), SDL_GetError());
    }
    _textures.emplace(path, tex);
    SDL_FreeSurface(surf);
    return tex;
}

} // namespace Ui
