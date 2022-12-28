#include "button.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define SDL_HAS_SCALE_MODE SDL_VERSION_ATLEAST(2, 0, 12)


namespace Ui {

Button::Button(int x, int y, int w, int h, FONT font, const std::string& text)
    : Label(x,y,w,h,font,text)
{
    // TODO: move padding to label?
    _autoSize.width += 2*_padding;
    _autoSize.height += 2*_padding;
    _minSize.width += 2*_padding;
    _minSize.height += 2*_padding;
    _size.width += 2*_padding;
    _size.height += 2*_padding;
    _backgroundColor = {96,96,96,96};
}

Button::~Button()
{
    if (_iconSurf) SDL_FreeSurface(_iconSurf);
    if (_iconTex) SDL_DestroyTexture(_iconTex);
}

void Button::setText(const std::string& text)
{
    Label::setText(text);
    _autoSize.width += 2*_padding;
    _autoSize.height += 2*_padding;
    _minSize.width += 2*_padding;
    _minSize.height += 2*_padding;
}

void Button::setIcon(const void* data, size_t len)
{
    if (_iconSurf || _iconTex) {
        if (_iconSurf) SDL_FreeSurface(_iconSurf);
        if (_iconTex) SDL_DestroyTexture(_iconTex);
        _iconTex = nullptr;
        _autoSize.width -= (ICON_SIZE + _padding);
        _minSize.width -= (ICON_SIZE + _padding);
    }

    _iconSurf = IMG_Load_RW(SDL_RWFromMem((void*)data, (int)len), 1);
    if (_iconSurf) {
        _autoSize.width += (ICON_SIZE + _padding);
        _minSize.width += (ICON_SIZE + _padding);
    }
}

void Button::render(Renderer renderer, int offX, int offY)
{
    if (_iconSurf && !_iconTex) {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "2");
        _iconTex = SDL_CreateTextureFromSurface(renderer, _iconSurf);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "");
        if (_iconTex) {
#if SDL_HAS_SCALE_MODE
            SDL_SetTextureScaleMode(_iconTex, SDL_ScaleModeBest);
#endif
            if (_iconSurf->w > _iconSurf->h) {
                _iconSize = {
                    ICON_SIZE,
                    ICON_SIZE * _iconSurf->h / _iconSurf->w
                };
            } else {
                _iconSize = {
                    ICON_SIZE * _iconSurf->w / _iconSurf->h,
                    ICON_SIZE
                };
            }
        }
        else {
            SDL_FreeSurface(_iconSurf);
            _iconSurf = nullptr;
            _autoSize.width -= (ICON_SIZE + _padding);
            _minSize.width -= (ICON_SIZE + _padding);
        }
    }

    int w = _iconSize.width;
    int h = _iconSize.height;
    int x = _padding + (ICON_SIZE - w)/2;
    int y = (_autoSize.height - h)/2;
    SDL_Rect dest = {.x = offX + _pos.left + x, .y = offY + _pos.top + y, .w = w, .h = h};
    SDL_RenderCopy(renderer, _iconTex, NULL, &dest);

    _autoSize.width -= 2*_padding;
    _autoSize.height -= 2*_padding;
    auto originalBgColor = _backgroundColor;
    
    if (getPressed()) {
        _backgroundColor.r/=2;
        _backgroundColor.g/=2;
        _backgroundColor.b/=2;
    }
    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, _size.height };
        SDL_RenderFillRect(renderer, &r);
    }

    if (_iconTex) offX += (ICON_SIZE + _padding)/2;
    _backgroundColor.a = 0;
    Label::render(renderer, offX, offY);
    
    // TODO: draw border
    
    _backgroundColor = originalBgColor;
    _autoSize.width += 2*_padding;
    _autoSize.height += 2*_padding;
}

} // namespace

