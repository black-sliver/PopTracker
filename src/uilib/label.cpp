#include "label.h"
#include <string.h>
#include "textutil.h"


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

void Label::render(Renderer renderer, int offX, int offY)
{
    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, _size.height };
        SDL_RenderFillRect(renderer, &r);
    }
    if (!_tex && !_text.empty() && _font) {
        SDL_Color color = {_textColor.r, _textColor.g, _textColor.b, 0xff};
        SDL_Surface* surf = RenderText(_font, _text.c_str(), color, _halign);
        if (surf) {
            _tex = SDL_CreateTextureFromSurface(renderer, surf);
            _autoSize = {surf->w, surf->h};
            SDL_FreeSurface(surf);
        } else {
            printf("Text render error: %s\n", TTF_GetError());
        }
    }
    if (!_tex) return;
    // TODO: gravity
    int x = offX + _pos.left;
    int mx = x + _size.width/2;
    int y = offY + _pos.top;
    int my = y + _size.height/2;    
    int texW = _autoSize.width;
    int texH = _autoSize.height;
    
    int destX;
    if (_halign == HAlign::CENTER)
        destX = mx-texW/2;
    else if (_halign == HAlign::LEFT)
        destX = x;
    else
        destX = x+_size.width-texW;
    int destY;
    if (_valign == VAlign::MIDDLE)
        destY = my-texH/2;
    else if (_valign == VAlign::TOP)
        destY = y;
    else
        destY = y+_size.height-texH;
    
    SDL_Rect dest { destX, destY, texW, texH };
    SDL_Rect src { 0,0,texW,texH };
    SDL_Rect *psrc = nullptr;
    if (_size.width>0 && dest.w>_size.width) {
        if (_halign == HAlign::RIGHT)
            dest.x += (dest.w - _size.width);
        else if (_halign == HAlign::CENTER)
            dest.x += (dest.w - _size.width)/2;
        dest.w = _size.width;
        src.w = _size.width;
        psrc = &src;
    }
    if (_size.height>0 && dest.h>_size.height) {
        if (_valign == VAlign::BOTTOM)
            dest.y += (dest.h - _size.height);
        else if (_halign == HAlign::CENTER)
            dest.y += (dest.h - _size.height)/2;
        dest.h = _size.height;
        src.w = _size.width;
        psrc = &src;
    }
    SDL_RenderCopy(renderer, _tex, psrc, &dest);
}

void Label::setText(const std::string& text)
{
    if (text == _text) return;
    _text = text;
    int autoW=0, autoH=0;
    if (_font && !_text.empty()) SizeText(_font, _text.c_str(), &autoW, &autoH);
    _autoSize = { autoW, autoH };
    _minSize = _autoSize; // until we support stretching or ellipsis
    if (_tex) SDL_DestroyTexture(_tex);
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
