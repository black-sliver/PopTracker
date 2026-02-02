#include "image.h"
#include <stdio.h>
#include <SDL2/SDL_image.h>
#include "colorhelper.h"
#include "imghelper.h"
#include "../core/fs.h"


namespace Ui {

Image::Image(int x, int y, int w, int h, const fs::path& path)
    : Widget(x,y,w,h)
{
    setImage(path);
    if (w<1 && h<1) {
        _size.width = _autoSize.width;
        _size.height = _autoSize.height;
    } else if (w<1 && _autoSize.height) {
        _size.width = (_autoSize.width * h + _autoSize.height/2) / _autoSize.height;
    } else if (h<1 && _autoSize.width) {
        _size.height = (_autoSize.height * w + _autoSize.width/2) / _autoSize.width;
    }
}

Image::Image(int x, int y, int w, int h, const void* data, size_t len)
    : Widget(x,y,w,h)
{
    setImage(data, len);
    if (w<1 && h<1) {
        _size.width = _autoSize.width;
        _size.height = _autoSize.height;
    } else if (w<1 && _autoSize.height) {
        _size.width = (_autoSize.width * h + _autoSize.height/2) / _autoSize.height;
    } else if (h<1 && _autoSize.width) {
        _size.height = (_autoSize.height * w + _autoSize.width/2) / _autoSize.width;
    }
}

Image::~Image()
{
    if (_tex)   SDL_DestroyTexture(_tex);
    if (_texBw) SDL_DestroyTexture(_texBw);
    if (_surf)  SDL_FreeSurface(_surf);
    _tex   = nullptr;
    _texBw = nullptr;
    _surf  = nullptr;
}

void Image::setImage(const fs::path& path)
{
    // NOTE: if the app hangs or crashes in IMG_Load, you are probably mixing incompatible DLLs
    // FIXME: loading images takes a majority of the time to build the UI. Cache it!
    if (path == _path && !path.empty())
        return; // unchanged

    if (_tex)   SDL_DestroyTexture(_tex);
    if (_texBw) SDL_DestroyTexture(_texBw);
    if (_surf)  SDL_FreeSurface(_surf);
    _tex   = nullptr;
    _texBw = nullptr;
    _surf  = nullptr;
    _autoSize = {0, 0};

    if (path.empty()) {
        _path.clear();
        return; // no data
    }
    _path = path;

    _surf = Ui::LoadImage(_path);
    if (_surf) {
        _autoSize = {_surf->w, _surf->h};
    } else {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
    }
}

void Image::setImage(const void* data, size_t len)
{
    // NOTE: if the app hangs or crashes in IMG_Load_RW, you are probably mixing incompatible DLLs
    // FIXME: loading images takes a majority of the time to build the UI. Cache it!
    if (_tex)   SDL_DestroyTexture(_tex);
    if (_texBw) SDL_DestroyTexture(_texBw);
    if (_surf)  SDL_FreeSurface(_surf);
    _tex   = nullptr;
    _texBw = nullptr;
    _surf  = nullptr;
    _autoSize = {0, 0};
    _path.clear();

    if (!data || !len)
        return; // no data

    _surf = IMG_Load_RW(SDL_RWFromMem((void*)data, (int)len), 1);
    if (_surf) {
        _autoSize = {_surf->w, _surf->h};
    } else {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
    }
}

void Image::ensureTexture(Renderer renderer)
{
    if (!_tex && _surf) {
        if (_quality >= 0) {
            // set Texture filter/quality when creating the texture
            char q[] = { (char)('0'+_quality), 0 };
            if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, q)) {
                printf("Image: could not set scale quality to %s!\n", q);
            }
        }
        _tex = SDL_CreateTextureFromSurface(renderer, _surf);
        _surf = makeGreyscale(_surf, _darkenGreyscale);
        _texBw = SDL_CreateTextureFromSurface(renderer, _surf);
        SDL_FreeSurface(_surf);
        if (_quality >= 0) {
            // TODO: have the default somewhere accessible?
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "");
        }
        _surf = nullptr;
    }
}

void Image::render(Renderer renderer, int offX, int offY)
{
    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, _size.height };
        SDL_RenderFillRect(renderer, &r);
    }
    ensureTexture(renderer);
    auto tex = _enabled ? _tex : _texBw;
    if (!tex) return;
    if (_fixedAspect) {
        int finalw=0, finalh=0;
        float ar = (float)_autoSize.width / (float)_autoSize.height;
        finalw = (int)(_size.height * ar + 0.5);
        finalh = _size.height;
        if (finalw > _size.width) {
            finalh = (int)(_size.width / ar + 0.5);
            finalw = _size.width;
        }
        SDL_Rect dest = {
            offX+_pos.left + (_size.width-finalw+1)/2,
            offY+_pos.top  + (_size.height-finalh+1)/2,
            finalw,
            finalh
        };
        SDL_RenderCopy(renderer, tex, NULL, &dest);
    } else {
        SDL_Rect dest = {offX+_pos.left, offY+_pos.top, _size.width, _size.height};
        SDL_RenderCopy(renderer, tex, NULL, &dest);
    }
}

void Image::setSize(Size size)
{
    // TODO: make this default behaviour in Widget::setSize() ?
    if (_size == size) return;
    if (size.width<_minSize.width) size.width=_minSize.width;
    if (size.height<_minSize.height) size.height=_minSize.height;
    Widget::setSize(size);
}

void Image::setDarkenGreyscale(bool value)
{
    if (_darkenGreyscale == value) return;
    _darkenGreyscale = value;
    if (_tex)   SDL_DestroyTexture(_tex);
    if (_texBw) SDL_DestroyTexture(_texBw);
    _tex = nullptr;
    _texBw = nullptr;
}

} // namespace
