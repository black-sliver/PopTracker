#include "item.h"
#include <assert.h>
#include <stdio.h>
#include <SDL2/SDL_image.h>
#include "../uilib/colorhelper.h"
#include "../uilib/imghelper.h"
#include "../uilib/textutil.h"


namespace Ui {

Item::Item(int x, int y, int w, int h, FONT font)
    : Widget(x,y,w,h)
{
    _font = font;
}

Item::~Item()
{
    clearImageOverride();
    for (auto& texset : _texs) for (auto& tex : texset) if (tex) SDL_DestroyTexture(tex);
    for (auto& surfset : _surfs) for (auto& surf : surfset) if (surf) SDL_FreeSurface(surf);
}

void Item::setStage(int stage1, int stage2)
{
    if (stage1>=0) _stage1 = stage1;
    if (stage2>=0) _stage2 = stage2;
}

void Item::freeStage(int stage1, int stage2)
{
    if ((int)_surfs.size() > stage1 && (int)_surfs[stage1].size() > stage2) {
        SDL_FreeSurface(_surfs[stage1][stage2]);
        _surfs[stage1][stage2] = nullptr;
    }
    if ((int)_texs.size() > stage1 && (int)_texs[stage1].size() > stage2) {
        SDL_DestroyTexture(_texs[stage1][stage2]);
        _texs[stage1][stage2] = nullptr;
    }
    if ((int)_names.size() > stage1 && (int)_names[stage1].size() > stage2) {
        _names[stage1][stage2].clear();
    }
    if ((int)_filters.size() > stage1 && (int)_filters[stage1].size() > stage2) {
        _filters[stage1][stage2].clear();
    }
    if (static_cast<int>(_futures.size()) > stage1 && static_cast<int>(_futures[stage1].size()) > stage2)
        _futures[stage1][stage2].reset();
}

static SDL_Surface* updateSurface(SDL_Surface* surf, const std::list<ImageFilter>& filters)
{
    if (!surf)
        return surf;
    // if any corner pixel is #ff00ff, make that color transparent
    surf = makeTransparent(surf, 0xff, 0x00, 0xff, filters.empty());
    if (!surf)
        return surf;
    // if filters have an overlay, we need an RGB(A) surface
    bool needsRGB = false;
    for (auto& filter: filters) {
        if (filter.name == "overlay") {
            needsRGB = true;
            break;
        }
    }
    if (needsRGB && surf->format->BitsPerPixel < 32) {
        SDL_Surface* old = surf;
        surf = SDL_ConvertSurfaceFormat(old, SDL_PIXELFORMAT_ARGB8888, 0);
        SDL_FreeSurface(old);
    }
    return surf;
}

void Item::updateSize(const Size size)
{
    // store size
    if (_autoSize.width < size.width) _autoSize.width = size.width;
    if (_autoSize.height < size.height) _autoSize.height = size.height;
    if (_size.width < 1 && _size.height < 1) {
        _size.width = _autoSize.width;
        _size.height = _autoSize.height;
    } else if (_size.width < 1 && _autoSize.height > 0) {
        _size.width = (_autoSize.width * _size.height + _autoSize.height / 2) / _autoSize.height;
    } else if (_size.height < 1 && _autoSize.width > 0) {
        _size.height = (_autoSize.height * _size.width + _autoSize.width / 2) / _autoSize.width;
    }
}

void Item::addStage(const int stage1, const int stage2, SDL_Surface* surf, const std::string& name,
                    const std::list<ImageFilter>& filters)
{
    surf = updateSurface(surf, filters);
    if (!surf)
        return;
    updateSize({surf->w, surf->h});
    while ((int)_surfs.size() <= stage1) {
        _surfs.push_back({});
        _names.push_back({});
        _filters.push_back({});
    }
    while ((int)_surfs[stage1].size() <= stage2) {
        _surfs[stage1].push_back(nullptr);
        _names[stage1].push_back("");
        _filters[stage1].push_back({});
    }
    // apply filters
    for (const auto& filter: filters)
        surf = filter.apply(surf);
    // store final surface
    _surfs[stage1][stage2] = surf;
    _names[stage1][stage2] = name;
    _filters[stage1][stage2] = filters;
}

void Item::addStage(const int stage1, const int stage2, const char *path, const std::list<ImageFilter>& filters)
{
    freeStage(stage1, stage2);
    if (!path || !*path) return;
    // FIXME: loading images takes a majority of the time to build the UI. Cache it!
    if (SDL_Surface* surf = IMG_Load(path))
        addStage(stage1, stage2, surf, path, filters);
    else
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
}

void Item::addStage(const int stage1, const int stage2, const void *data, size_t len, const std::string& name,
                    const std::list<ImageFilter>& filters)
{
    freeStage(stage1, stage2);
    if (!data || !len) return;
    // FIXME: loading images takes a majority of the time to build the UI. Cache it!
    if (SDL_Surface* surf = IMG_Load_RW(SDL_RWFromMem(const_cast<void*>(data), static_cast<int>(len)), 1))
        addStage(stage1, stage2, surf, name, filters);
    else
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
}

void Item::addStage(const int stage1, const int stage2, std::unique_ptr<ImageFuture> &&future, std::string name,
    std::list<ImageFilter> filters)
{
    freeStage(stage1, stage2);
    if (!future)
        return;
    const Size size = future->getSize();
    if (size.width < 1 || size.height < 1)
        return; // bad image
    updateSize(size);
    if (static_cast<int>(_futures.size()) <= stage1)
        _futures.resize(stage1 + 1);
    if (static_cast<int>(_futures[stage1].size()) <= stage2)
        _futures[stage1].resize(stage2 + 1);
    if (static_cast<int>(_names.size()) <= stage1)
        _names.resize(stage1 + 1);
    if (static_cast<int>(_names[stage1].size()) <= stage2)
        _names[stage1].resize(stage2 + 1);
    if (static_cast<int>(_filters.size()) <= stage1)
        _filters.resize(stage1 + 1);
    if (static_cast<int>(_filters[stage1].size()) <= stage2)
        _filters[stage1].resize(stage2 + 1);
    _futures[stage1][stage2] = std::move(future);
    _filters[stage1][stage2] = std::move(filters);
    _names[stage1][stage2] = std::move(name);
}

bool Item::isStage(int stage1, int stage2, const std::string& name, std::list<ImageFilter> filters)
{
    if (name.empty() || (int)_names.size() <= stage1 || (int)_filters.size() <= stage1 ||
            (int)_names[stage1].size() <= stage2 || (int)_filters[stage1].size() <= stage2)
        return false;
    return _names[stage1][stage2] == name && _filters[stage1][stage2] == filters;
}

void Item::render(Renderer renderer, int offX, int offY)
{
    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect r = { offX+_pos.left-_margin.left,
                       offY+_pos.top-_margin.top,
                       _size.width+_margin.left+_margin.right,
                       _size.height+_margin.top+_margin.bottom
        };
        SDL_RenderFillRect(renderer, &r);
    }
    assert(_stage1 >= 0 && _stage2 >= 0);
    // resolve futures
    if (_stage1 < static_cast<int>(_futures.size()) && _stage2 < static_cast<int>(_futures[_stage1].size())) {
        if (auto& future = _futures[_stage1][_stage2]) {
            // TODO: move filter resolution into the future
            if (isSmallImage(future->getSize()) || future->isSurfaceDone()) {
                SDL_Surface* surf = future->getSurface();
                future.reset();
                surf = updateSurface(surf, _filters[_stage1][_stage2]);
                for (const auto& filter: _filters[_stage1][_stage2])
                    surf = filter.apply(surf);
                if (static_cast<int>(_surfs.size()) <= _stage1)
                    _surfs.resize(_stage1 + 1);
                if (static_cast<int>(_surfs[_stage1].size()) <= _stage2)
                    _surfs[_stage1].resize(_stage2 + 1);
                _surfs[_stage1][_stage2] = surf;
            } else {
                future->prioritize();
            }
        }
    }
    if (_overrideFuture) {
        if (isSmallImage(_overrideFuture->getSize()) || _overrideFuture->isSurfaceDone()) {
            SDL_Surface* surf = _overrideFuture->getSurface();
            _overrideFuture.reset();
            surf = updateSurface(surf, _overrideFilters);
            for (auto& filter: _overrideFilters)
                surf = filter.apply(surf);
            if (surf) {
                if (_overrideSurf)
                    SDL_FreeSurface(_overrideSurf); // free the placeholder surface
                if (_overrideTex)
                    SDL_DestroyTexture(_overrideTex); // free the placeholder texture
                _overrideTex = nullptr;
                _overrideSurf = surf;
            }
        } else {
            _overrideFuture->prioritize();
        }
    }
    // get surface/texture
    const bool overridden = _overrideTex || _overrideSurf || _overrideFuture;
    auto tex  = overridden ? _overrideTex
            : (_stage1<(int)_texs.size() && _stage2<(int)_texs[_stage1].size()) ? _texs[_stage1][_stage2]
            : nullptr;
    auto surf = tex ? nullptr
            : overridden ? _overrideSurf
            : (_stage1<(int)_surfs.size() && _stage2<(int)_surfs[_stage1].size()) ? _surfs[_stage1][_stage2]
            : nullptr;
    if (!tex && surf) {
        if (_quality >= 0) {
            // set Texture filter/quality when creating the texture
            char q[] = { static_cast<char>('0' + _quality), 0 };
            if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, q)) {
                printf("Item: could not set scale quality to %s!\n", q);
            }
        }
        tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface (surf);
        if (overridden) {
            _overrideTex = tex;
            _overrideSurf = nullptr;
            surf = nullptr;
        } else {
            _surfs[_stage1][_stage2] = nullptr;
            surf = nullptr;
            if (static_cast<int>(_texs.size()) <= _stage1)
                _texs.resize(_stage1 + 1);
            if (static_cast<int>(_texs[_stage1].size()) <= _stage2)
                _texs[_stage1].resize(_stage2 + 1);
            _texs[_stage1][_stage2] = tex;
        }
        if (_quality >= 0) {
            // TODO: have the default somewhere accessible?
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "");
        }
    }
    if (!tex)
        return;
    if (_fixedAspect) {
        int finalw=0, finalh=0;
        float ar = (float)_autoSize.width / (float)_autoSize.height;
        finalw = (int)(_size.height * ar + 0.5);
        finalh = _size.height;
        if (finalw > _size.width) {
            finalh = (int)(_size.width / ar + 0.5);
            finalw = _size.width;
        }
        _renderSize = { finalw, finalh };
        int xalign = _halign == Label::HAlign::LEFT ? 0 : (_size.width-finalw+1);
        if (_halign != Label::HAlign::RIGHT) xalign /= 2;
        int yalign = _valign == Label::VAlign::TOP ? 0 : (_size.height-finalh+1);
        if (_valign != Label::VAlign::BOTTOM) yalign /= 2;
        _renderPos = {
            _pos.left + xalign,
            _pos.top + yalign,
        };
    } else {
        _renderSize = _size;
        _renderPos = _pos;
    }
    SDL_Rect dest = {offX+_renderPos.left, offY+_renderPos.top, _renderSize.width, _renderSize.height};
    SDL_RenderCopy(renderer, tex, NULL, &dest);
    if (!_overlay.empty() && _font && !_overlayTex) {
        // text
        SDL_Surface* tsurf = RenderText(_font, _overlay.c_str(), {
            _overlayColor.r, _overlayColor.g, _overlayColor.b, _overlayColor.a
        }, Label::HAlign::RIGHT);
        // light
        SDL_Surface* lsurf = RenderText(_font, _overlay.c_str(), {
            255, 255, 255, 255
        }, Label::HAlign::RIGHT);
        SDL_SetSurfaceAlphaMod(lsurf, 96);
        // shadow
        SDL_Surface* ssurf = RenderText(_font, _overlay.c_str(), {
            0, 0, 0, 255
        }, Label::HAlign::RIGHT);
        // combine
        SDL_Surface* surf = nullptr;
        if (tsurf && lsurf && ssurf) {
            surf = SDL_CreateRGBSurfaceWithFormat(0, tsurf->w+2, tsurf->h+2, tsurf->format->BitsPerPixel, tsurf->format->format);
            if (surf) {
                SDL_Rect d = { 0, 0, tsurf->w, tsurf->h };
                if (_overlayBackgroundColor.a)
                    SDL_FillRect(lsurf, &d, SDL_MapRGBA(lsurf->format,
                            _overlayBackgroundColor.r, _overlayBackgroundColor.g,
                            _overlayBackgroundColor.b, _overlayBackgroundColor.a
                    ));
                SDL_BlitSurface(lsurf, NULL, surf, &d);
                SDL_SetSurfaceAlphaMod(lsurf, 128);
                d.x=1; d.y=0;
                SDL_BlitSurface(lsurf, NULL, surf, &d);
                d.x=0; d.y=1;
                SDL_BlitSurface(lsurf, NULL, surf, &d);
                d.x=2; d.y=2;
                SDL_BlitSurface(ssurf, NULL, surf, &d);
                d.x=1; d.y=1;
                SDL_BlitSurface(tsurf, NULL, surf, &d);
            }
        }
        if (tsurf) SDL_FreeSurface(tsurf);
        if (ssurf) SDL_FreeSurface(ssurf);
        if (lsurf) SDL_FreeSurface(lsurf);
        if (surf) {
            _overlayTex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_FreeSurface(surf);
        } else {
            printf("Text render error: %s\n", TTF_GetError());
        }
    }
    if (_overlayTex) {
        int ow=0,oh=0; // TODO: cache instead
        if (SDL_QueryTexture(_overlayTex, NULL, NULL, &ow, &oh) == 0) {
            SDL_Rect dest;
            int bottom = offY+_pos.top+_size.height-1;
            if (_overlayAlign == Label::HAlign::CENTER) {
                int center = offX+_pos.left+_size.width/2;
                dest = {
                    center-ow/2,
                    bottom-oh,
                    ow,
                    oh
                };
            } else if (_overlayAlign == Label::HAlign::LEFT) {
                int left = offX+_pos.left;
                dest = {
                    left,
                    bottom-oh,
                    ow,
                    oh
                };
            } else {
                int right = offX+_pos.left+_size.width-1;
                dest = {
                    right-ow,
                    bottom-oh,
                    ow,
                    oh
                };
            }
            SDL_RenderCopy(renderer, _overlayTex, NULL, &dest);
        }
    }
}

void Item::setSize(Size size) {
    // TODO: make this default behavout in Widget::setSize() ?
    if (_size == size) return;
    if (size.width<_minSize.width) size.width=_minSize.width;
    if (size.height<_minSize.height) size.height=_minSize.height;
    Widget::setSize(size);
}

void Item::setFont(Item::FONT font) {
    if (_font == font) return;
    if (_overlayTex) SDL_DestroyTexture(_overlayTex);
    _overlayTex = nullptr;
    _font = font;
}

void Item::setOverlay(const std::string& s) {
    if (s == _overlay) return;
    if (_overlayTex) SDL_DestroyTexture(_overlayTex);
    _overlayTex = nullptr;
    _overlay = s;
}

void Item::setOverlayColor(Widget::Color c) {
    if (c == _overlayColor) return;
    if (_overlayTex) SDL_DestroyTexture(_overlayTex);
    _overlayTex = nullptr;
    _overlayColor = c;
}

void Item::setOverlayBackgroundColor(Widget::Color c)
{
    if (c == _overlayBackgroundColor) return;
    if (_overlayTex) SDL_DestroyTexture(_overlayTex);
    _overlayTex = nullptr;
    _overlayBackgroundColor = c;
}

void Item::setOverlayAlignment(Label::HAlign halign)
{
    if (halign == _overlayAlign) return;
    if (_overlayTex) SDL_DestroyTexture(_overlayTex);
    _overlayTex = nullptr;
    _overlayAlign = halign;
}

void Item::setImageOverride(const void *data, const size_t len, const std::string& name,
    const std::list<ImageFilter>& filters)
{
    const bool overridden = _overrideSurf || _overrideTex || _overrideFuture;
    if (overridden && name == _overrideName && filters == _overrideFilters)
        return;

    clearImageOverride();

    SDL_Surface* surf = nullptr;
    if (data && len) {
        surf = IMG_Load_RW(SDL_RWFromMem(const_cast<void *>(data), static_cast<int>(len)), 1);
        if (!surf) {
            fprintf(stderr, "Could not load image: %s\n", SDL_GetError());
        } else {
            surf = updateSurface(surf, filters);
            for (auto& filter: filters)
                surf = filter.apply(surf);
        }
    }

    if (!surf) {
        // empty placeholder
        static constexpr uint32_t zero = 0;
        auto* zeroPtr = const_cast<void*>(static_cast<const void*>(&zero));
        surf = SDL_CreateRGBSurfaceWithFormatFrom(zeroPtr, 1, 1, 32, 4, SDL_PIXELFORMAT_ARGB8888);
    }

    _overrideName = name;
    _overrideFilters = filters;
    _overrideSurf = surf;
}

void Item::setImageOverride(const std::function<std::unique_ptr<ImageFuture>(void)>& generator, const std::string &name,
    const std::list<ImageFilter> &filters)
{
    const bool overridden = _overrideSurf || _overrideTex || _overrideFuture;
    if (overridden && name == _overrideName && filters == _overrideFilters)
        return;

    clearImageOverride();

    _overrideName = name;
    _overrideFilters = filters;
    _overrideFuture = generator();
    // create placeholder override surface
    static constexpr uint32_t zero = 0;
    auto* zeroPtr = const_cast<void*>(static_cast<const void*>(&zero));
    _overrideSurf = SDL_CreateRGBSurfaceWithFormatFrom(zeroPtr, 1, 1, 32, 4, SDL_PIXELFORMAT_ARGB8888);
}

void Item::clearImageOverride()
{
    if (_overrideFuture)
        _overrideFuture.reset();
    if (_overrideTex) {
        SDL_DestroyTexture(_overrideTex);
        _overrideTex = nullptr;
    }
    if (_overrideSurf) {
        SDL_FreeSurface(_overrideSurf);
        _overrideSurf = nullptr;
    }
    _overrideName.clear();
    _overrideFilters.clear();
}

} // namespace
