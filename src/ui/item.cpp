#include "item.h"
#include <stdio.h>
#include <SDL_image.h>
#include "../uilib/colorhelper.h"


namespace Ui {

Item::Item(int x, int y, int w, int h, FONT font)
    : Widget(x,y,w,h)
{
    _font = font;
}

Item::~Item()
{
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
    if ((int)_surfs.size()>stage1 && (int)_surfs[stage1].size()>stage2) {
        SDL_FreeSurface(_surfs[stage1][stage2]);
        _surfs[stage1][stage2] = nullptr;
    }
    if ((int)_texs.size()>stage1 && (int)_texs[stage1].size()>stage2) {
        SDL_DestroyTexture(_texs[stage1][stage2]);
        _texs[stage1][stage2] = nullptr;
    }
}
void Item::addStage(int stage1, int stage2, SDL_Surface* surf, std::list<ImageFilter> filters)
{
    if (!surf) return;
    // if any corner pixel is #ff00ff, make that color transparent
    surf = makeTransparent(surf, 0xff, 0x00, 0xff, filters.empty());
    // store size
    if (_autoSize.width  < surf->w) _autoSize.width  = surf->w;
    if (_autoSize.height < surf->h) _autoSize.height = surf->h;
    if (_size.width<1 && _size.height<1) {
        _size.width = _autoSize.width;
        _size.height = _autoSize.height;
    } else if (_size.width<1) {
        _size.width = (_autoSize.width * _size.height + _autoSize.height/2) / _autoSize.height;
    } else if (_size.height<1) {
        _size.height = (_autoSize.height * _size.width + _autoSize.width/2) / _autoSize.width;
    }
    while ((int)_surfs.size() <= stage1) _surfs.push_back({});
    while ((int)_surfs[stage1].size() <= stage2) _surfs[stage1].push_back(nullptr);
    // apply filters
    for (auto filter: filters)
        surf = filter.apply(surf);
    // store final surface
    _surfs[stage1][stage2] = surf;
}
void Item::addStage(int stage1, int stage2, const char *path, std::list<ImageFilter> filters)
{
    freeStage(stage1, stage2);
    if (!path || !*path) return;
    auto surf = IMG_Load(path);
    if (surf) {
        addStage(stage1, stage2, surf, filters);
    }
    else {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
    }
}
void Item::addStage(int stage1, int stage2, const void *data, size_t len, std::list<ImageFilter> filters)
{
    freeStage(stage1, stage2);
    if (!data || !len) return;
    auto surf = IMG_Load_RW(SDL_RWFromMem((void*)data, (int)len), 1);
    if (surf) {
        addStage(stage1, stage2, surf, filters);
    }
    else {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
    }
}

void Item::render(Renderer renderer, int offX, int offY)
{
    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, _size.height };
        SDL_RenderFillRect(renderer, &r);
    }
    auto tex  = (_stage1<(int)_texs.size() && _stage2<(int)_texs[_stage1].size()) ? _texs[_stage1][_stage2] : nullptr;
    auto surf = (!tex && _stage1<(int)_surfs.size() && _stage2<(int)_surfs[_stage1].size()) ? _surfs[_stage1][_stage2] : nullptr;
    if (!tex && surf) {
        if (_quality >= 0) {
            // set Texture filter/quality when creating the texture
            char q[] = { (char)('0'+_quality), 0 };
            if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, q)) {
                printf("Image: could not set scale quality to %s!\n", q);
            }
        }
        tex = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface (surf);
        _surfs[_stage1][_stage2] = nullptr;
        if (_quality >= 0) {
            // TODO: have the default somewhere accessible?
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "");
        }
        surf = nullptr;
        while ((int)_texs.size() <= _stage1) _texs.push_back({});
        while ((int)_texs[_stage1].size() <= _stage2) _texs[_stage1].push_back(nullptr);
        _texs[_stage1][_stage2] = tex;
    }
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
    if (!_overlay.empty() && _font && !_overlayTex) {
        // text
        SDL_Surface* tsurf = TTF_RenderUTF8_Blended(_font, _overlay.c_str(), {
            _overlayColor.r, _overlayColor.g, _overlayColor.b, _overlayColor.a
        });
        // light
        SDL_Surface* lsurf = TTF_RenderUTF8_Blended(_font, _overlay.c_str(), {
            255, 255, 255, 255
        });
        SDL_SetSurfaceAlphaMod(lsurf, 96);
        // shadow
        SDL_Surface* ssurf = TTF_RenderUTF8_Blended(_font, _overlay.c_str(), {
            0, 0, 0, 255
        });
        // combine
        SDL_Surface* surf = nullptr;
        if (tsurf && lsurf && ssurf) {
            surf = SDL_CreateRGBSurfaceWithFormat(0, tsurf->w+2, tsurf->h+2, tsurf->format->BitsPerPixel, tsurf->format->format);
            if (surf) {
                SDL_Rect d = { .x=0, .y=0, .w=tsurf->w, .h=tsurf->h };
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
            if (ow>_size.width) {
                int center = offX+_pos.left+_size.width/2;
                dest = {
                    .x = center-ow/2,
                    .y = bottom-oh,
                    .w = ow,
                    .h = oh
                };
            } else {
                int right = offX+_pos.left+_size.width-1;
                dest = {
                    .x = right-ow,
                    .y = bottom-oh,
                    .w = ow,
                    .h = oh
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

} // namespace
