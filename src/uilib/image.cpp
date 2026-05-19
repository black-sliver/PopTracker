#include "image.h"
#include <cstdio>
#include <SDL2/SDL_image.h>
#include "colorhelper.h"
#include "imghelper.h"
#include "../core/fs.h"


namespace Ui {

Image::Image(const int x, const int y, const int w, const int h, const fs::path& path)
    : Widget(x,y,w,h)
{
    setImage(path);
    if (w < 1 && h < 1) {
        _size.width = _autoSize.width;
        _size.height = _autoSize.height;
    } else if (w < 1 && _autoSize.height) {
        _size.width = (_autoSize.width * h + _autoSize.height / 2) / _autoSize.height;
    } else if (h < 1 && _autoSize.width) {
        _size.height = (_autoSize.height * w + _autoSize.width / 2) / _autoSize.width;
    }
}

Image::Image(const int x, const int y, const int w, const int h, const void* data, const size_t len)
    : Widget(x,y,w,h)
{
    setImage(data, len);
    if (w < 1 && h < 1) {
        _size.width = _autoSize.width;
        _size.height = _autoSize.height;
    } else if (w < 1 && _autoSize.height) {
        _size.width = (_autoSize.width * h + _autoSize.height / 2) / _autoSize.height;
    } else if (h < 1 && _autoSize.width) {
        _size.height = (_autoSize.height * w + _autoSize.width / 2) / _autoSize.width;
    }
}

Image::Image(const int x, const int y, const int w, const int h, std::unique_ptr<ImageFuture>&& future)
    : Widget(x,y,w,h)
{
    setImage(std::move(future));
    if (w < 1 && h < 1) {
        _size.width = _autoSize.width;
        _size.height = _autoSize.height;
    } else if (w < 1 && _autoSize.height) {
        _size.width = (_autoSize.width * h + _autoSize.height / 2) / _autoSize.height;
    } else if (h < 1 && _autoSize.width) {
        _size.height = (_autoSize.height * w + _autoSize.width / 2) / _autoSize.width;
    }
}

Image::~Image()
{
    clearImage();
}

void Image::setImage(const fs::path& path)
{
    // NOTE: if the app hangs or crashes in IMG_Load, you are probably mixing incompatible DLLs
    // FIXME: loading images takes a majority of the time to build the UI. Cache it!
    if (path == _path && !path.empty())
        return; // unchanged

    clearImage();

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

void Image::setImage(const void* data, const size_t len)
{
    // NOTE: if the app hangs or crashes in IMG_Load_RW, you are probably mixing incompatible DLLs
    // FIXME: loading images takes a majority of the time to build the UI. Cache it!
    clearImage();

    if (!data || !len)
        return; // no data

    _surf = IMG_Load_RW(SDL_RWFromMem(const_cast<void *>(data), static_cast<int>(len)), 1);
    if (_surf) {
        _autoSize = {_surf->w, _surf->h};
    } else {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
    }
}

void Image::setImage(std::unique_ptr<ImageFuture>&& future)
{
    // FIXME: loading images takes a majority of the time to build the UI. Cache it!
    clearImage();

    _future = std::move(future);
    const Size size = _future->getSize();
    if (size != Size::UNDEFINED)
        _autoSize = size;
}

void Image::clearImage()
{
    _future.reset();
    if (_tex)
        SDL_DestroyTexture(_tex);
    if (_texBw)
        SDL_DestroyTexture(_texBw);
    SDL_FreeSurface(_surf);
    _tex = nullptr;
    _texBw = nullptr;
    _surf = nullptr;
    _autoSize = {0, 0};
    _path.clear();
}

static bool isSmallImage(const Size size)
{
    // We decompress small images in the foreground to avoid blinkiness if possible.
    // To make this less costly, they can be cached in the provider.
    return size.width <= 4096 && size.height <= 4096 && size.width * size.height <= 4096;
}

void Image::ensureTexture(Renderer renderer, const bool lazy)
{
    if (_future && (!lazy || _future->isSurfaceDone() || isSmallImage(_autoSize))) {
        _surf = _future->getSurface();
        _future.reset();
    } else if (_future) {
        _future->prioritize();
    }
    if (!_tex && _surf) {
        if (_quality >= 0) {
            // set Texture filter/quality when creating the texture
            char q[] = { static_cast<char>('0' + _quality), 0 };
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

void Image::render(Renderer renderer, const int offX, const int offY)
{
    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        const SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, _size.height };
        SDL_RenderFillRect(renderer, &r);
    }
    ensureTexture(renderer);
    const auto tex = _enabled ? _tex : _texBw;
    if (!tex)
        return;
    if (_fixedAspect) {
        int finalW=0, finalH=0;
        const float ar = static_cast<float>(_autoSize.width) / static_cast<float>(_autoSize.height);
        finalW = static_cast<int>(_size.height * ar + 0.5);
        finalH = _size.height;
        if (finalW > _size.width) {
            finalH = static_cast<int>(_size.width / ar + 0.5);
            finalW = _size.width;
        }
        const SDL_Rect dest = {
            offX+_pos.left + (_size.width - finalW + 1) / 2,
            offY+_pos.top + (_size.height - finalH + 1) / 2,
            finalW,
            finalH
        };
        SDL_RenderCopy(renderer, tex, nullptr, &dest);
    } else {
        const SDL_Rect dest = {offX+_pos.left, offY+_pos.top, _size.width, _size.height};
        SDL_RenderCopy(renderer, tex, nullptr, &dest);
    }
}

void Image::setSize(Size size)
{
    // TODO: make this default behaviour in Widget::setSize() ?
    if (_size == size)
        return;
    if (size.width < _minSize.width)
        size.width = _minSize.width;
    if (size.height < _minSize.height)
        size.height = _minSize.height;
    Widget::setSize(size);
}

void Image::setDarkenGreyscale(const bool value)
{
    if (_darkenGreyscale == value)
        return;
    _darkenGreyscale = value;
    if (_tex)
        SDL_DestroyTexture(_tex);
    if (_texBw)
        SDL_DestroyTexture(_texBw);
    _tex = nullptr;
    _texBw = nullptr;
}

} // namespace
