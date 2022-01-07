#include "image.h"
#include <stdio.h>
#include <SDL2/SDL_image.h>
#include "colorhelper.h"

namespace Ui {

Image::Image(int x, int y, int w, int h, const char* path)
    : Widget(x,y,w,h), _path(path)
{
    if (!path || !*path) {
        _surf = nullptr;
        return;
    }
    _surf = IMG_Load(_path.c_str());
    if (_surf) {
        _autoSize = {_surf->w, _surf->h};
        if (w<1 && h<1) {
            _size.width = _autoSize.width;
            _size.height = _autoSize.height;
        } else if (w<1) {
            _size.width = (_autoSize.width * h + _autoSize.height/2) / _autoSize.height;
        } else if (h<1) {
            _size.height = (_autoSize.height * w + _autoSize.width/2) / _autoSize.width;
        }
    }
    else {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
    }
}
Image::Image(int x, int y, int w, int h, const void* data, size_t len)
    : Widget(x,y,w,h)
{
    if (!data || !len) {
        _surf = nullptr;
        return;
    }
    _surf = IMG_Load_RW(SDL_RWFromMem((void*)data, (int)len), 1);
    if (_surf) {
        _autoSize = {_surf->w, _surf->h};
        if (w<1 && h<1) {
            _size.width = _autoSize.width;
            _size.height = _autoSize.height;
        } else if (w<1) {
            _size.width = (_autoSize.width * h + _autoSize.height/2) / _autoSize.height;
        } else if (h<1) {
            _size.height = (_autoSize.height * w + _autoSize.width/2) / _autoSize.width;
        }
    }
    else {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
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

void Image::render(Renderer renderer, int offX, int offY)
{
    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, _size.height };
        SDL_RenderFillRect(renderer, &r);
    }
    if (!_tex && _surf) {
        if (_quality >= 0) {
            // set Texture filter/quality when creating the texture
            char q[] = { (char)('0'+_quality), 0 };
            if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, q)) {
                printf("Image: could not set scale quality to %s!\n", q);
            }
        }
        _tex = SDL_CreateTextureFromSurface(renderer, _surf);
        _surf = makeGreyscale(_surf);
        _texBw = SDL_CreateTextureFromSurface(renderer, _surf);
        SDL_FreeSurface (_surf);
        if (_quality >= 0) {
            // TODO: have the default somewhere accessible?
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "");
        }
        _surf = nullptr;
    }
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
            .x = offX+_pos.left + (_size.width-finalw+1)/2,
            .y = offY+_pos.top  + (_size.height-finalh+1)/2,
            .w = finalw,
            .h = finalh
        };
        SDL_RenderCopy(renderer, tex, NULL, &dest);
    } else {
        SDL_Rect dest = {.x = offX+_pos.left, .y = offY+_pos.top, .w = _size.width, .h = _size.height};
        SDL_RenderCopy(renderer, tex, NULL, &dest);
    }
}

void Image::setSize(Size size) {
    // TODO: make this default behavout in Widget::setSize() ?
    if (_size == size) return;
    if (size.width<_minSize.width) size.width=_minSize.width;
    if (size.height<_minSize.height) size.height=_minSize.height;
    Widget::setSize(size);
}

} // namespace
