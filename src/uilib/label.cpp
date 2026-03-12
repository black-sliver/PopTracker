#include "label.h"
#include <string.h>
#include "textutil.h"
#include "../ui/defaults.h"


namespace Ui {

Label::Label(int x, int y, int w, int h, FONT font, const std::string& text)
    : Widget(x,y,w,h), _font(font), _text(text)
{
    int autoW=0, autoH=0;
    if (_font && !_text.empty()) SizeText(_font, _text.c_str(), &autoW, &autoH);
    _autoSize = { autoW, autoH };
    _minSize = _autoSize; // until we support stretching or ellipsis
}
Label::~Label()
{
    if (_tex) SDL_DestroyTexture(_tex);
    _tex = nullptr;
}

static bool getTextureSize(SDL_Texture* tex, int* w, int* h)
{
    // TODO: perf test this
    return SDL_QueryTexture(tex, nullptr, nullptr, w, h) == 0;
}

void Label::render(Renderer renderer, int offX, int offY)
{
    float zoom;
    SDL_RenderGetScale(renderer, &zoom, nullptr);

    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, _size.height };
        SDL_RenderFillRect(renderer, &r);
    }
    int texW = 0, texH = 0;

    if (!_tex && !_text.empty() && _font) {
        //const float newFontSize = static_cast<float>(DEFAULT_FONT_SIZE) * zoom;
        //TTF_SetFontSize(_font, static_cast<int>(std::floor(newFontSize)));
        const unsigned newDpi = static_cast<unsigned>(std::round(72 * zoom));
        TTF_SetFontSizeDPI(_font, DEFAULT_FONT_SIZE, newDpi, newDpi);
        const SDL_Color color = {_textColor.r, _textColor.g, _textColor.b, _textColor.a};
        SDL_Surface* surf = RenderText(_font, _text.c_str(), color, _halign);
        TTF_SetFontSize(_font, DEFAULT_FONT_SIZE);
        if (surf) {
            _tex = SDL_CreateTextureFromSurface(renderer, surf);
            if (zoom != 1.0f)
                SDL_SetTextureScaleMode(_tex, SDL_ScaleModeBest);
            texW = surf->w;
            texH = surf->h;
            _autoSize = Size{texW, texH} / zoom;
            SDL_FreeSurface(surf);
        } else {
            printf("Text render error: %s\n", TTF_GetError());
        }
    } else if (_tex) {
        if (!getTextureSize(_tex, &texW, &texH)) {
            printf("Error getting texture size: %s\n", SDL_GetError());
            return;
        }
    }
    if (!_tex)
        return;

    const int x = offX + _pos.left;
    const int mx = x + _size.width / 2;
    const int y = offY + _pos.top;
    const int my = y + _size.height / 2;

    int destX;
    if (_halign == HAlign::CENTER)
        destX = mx - _autoSize.width / 2;
    else if (_halign == HAlign::LEFT)
        destX = x;
    else
        destX = x + _size.width - _autoSize.width;

    int destY;
    if (_valign == VAlign::MIDDLE)
        destY = my - _autoSize.height / 2;
    else if (_valign == VAlign::TOP)
        destY = y;
    else
        destY = y + _size.height - _autoSize.height;

    SDL_FRect destF {
        static_cast<float>(destX),
        static_cast<float>(destY),
        static_cast<float>(texW) / zoom,
        static_cast<float>(texH) / zoom,
    };
    SDL_Rect src { 0, 0, texW, texH };
    const SDL_Rect* pSrc = nullptr;
    const auto widthF = static_cast<float>(_size.width);
    const auto heightF = static_cast<float>(_size.height);
    if (_size.width > 0 && destF.w > widthF) {
        if (_halign == HAlign::RIGHT)
            destF.x += (destF.w - widthF);
        else if (_halign == HAlign::CENTER)
            destF.x += (destF.w - widthF) / 2;
        destF.w = widthF;
        src.w = static_cast<int>(std::round(static_cast<float>(_size.width) * zoom));
        pSrc = &src;
    }
    if (_size.height > 0 && destF.h > heightF) {
        if (_valign == VAlign::BOTTOM)
            destF.y += (destF.h - heightF);
        else if (_halign == HAlign::CENTER)
            destF.y += (destF.h - heightF) / 2;
        destF.h = heightF;
        src.h = static_cast<int>(std::round(static_cast<float>(_size.height) * zoom));
        pSrc = &src;
    }

    SDL_RenderCopyF(renderer, _tex, pSrc, &destF);
}

void Label::setText(const std::string& text)
{
    if (text == _text)
        return;
    _text = text;
    int autoW=0, autoH=0;
    if (_font && !_text.empty())
        SizeText(_font, _text.c_str(), &autoW, &autoH);
    _autoSize = { autoW, autoH };
    _minSize = _autoSize; // until we support stretching or ellipsis
    if (_tex)
        SDL_DestroyTexture(_tex);
    _tex = nullptr;
}

void Label::setTextColor(Widget::Color c)
{
    if (_textColor == c) return;
    _textColor = c;
    if (_tex) SDL_DestroyTexture(_tex);
    _tex = nullptr;
}


} // namespace
