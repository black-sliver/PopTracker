#include "label.h"
#include <string.h>


namespace Ui {

static inline SDL_Surface* _RenderText(Label::FONT font, const char* text,
        SDL_Color color, Label::HAlign halign,
        int passes=2, int* w=nullptr, int* h=nullptr, int* result=nullptr)
{
    if (w) *w = 0;
    if (h) *h = 0;
    if (result) *result = 0;
    const char* firstlf = strchr(text, '\n');
    SDL_Surface* surf = nullptr;
    if (!firstlf && passes>1) {
        surf = TTF_RenderUTF8_Blended(font, text, color);
        if (surf && w) *w = surf->w;
        if (surf && h) *h = surf->h;
        return surf;
    } else if (!firstlf) {
        int res = TTF_SizeUTF8(font, text, w, h);
        if (result) *result = res;
        return nullptr;
    }
    char* buf = strdup(text);
    int maxW = 0, totalH = 0, curY = 0;
    int res = 0;
    int linespace = 1; // TODO: line pitch instead?
    for (int pass=0; pass<passes; pass++) {
        char* lf = buf+(firstlf-text);
        char* p = buf;
        if (pass == 1) {
            // pass1: render text
            // create surface in same format as TTF_*
            surf = SDL_CreateRGBSurfaceWithFormat(0, maxW, totalH, 32,
                    SDL_PIXELFORMAT_ARGB8888);
            if (!surf) goto err;
        }
        while (true) {
            // if not end of string, insert NUL at \n and optionally \r
            if (lf) {
                *lf = '\0';
                if (lf>buf && *(lf-1)=='\r') *(lf-1) = 0;
            }
            if (pass == 0) {
                // pass0: measure text
                int w=0,h=0;
                res = TTF_SizeUTF8(font, p, &w, &h);
                if (res != 0) goto err;
                if (w>maxW) maxW=w;
                totalH += h + linespace;
            } else {
                // pass1: render text
                SDL_Surface* line = TTF_RenderUTF8_Blended(font, p, color);
                if (line) {
                    SDL_SetSurfaceBlendMode(line, SDL_BLENDMODE_NONE); // copy alpha value instead of blending
                    SDL_Rect rect = { 0, curY, line->w, line->h };
                    if (halign == Label::HAlign::RIGHT)
                        rect.x = maxW - rect.w;
                    else if (halign == Label::HAlign::CENTER)
                        rect.x = (maxW - rect.w)/2;
                    SDL_BlitSurface(line, nullptr, surf, &rect);
                    curY += line->h + linespace;
                    SDL_FreeSurface(line);
                }
            }
            if (!lf) break; // pass done
            // restore string for next pass
            if (pass < passes-1) {
                *lf = '\n';
                if (lf>buf && *(lf-1)=='\0') *(lf-1) = '\r';
            }
            // find next \n
            p = lf+1;
            lf = strchr(p, '\n');
        }
        // undo space after last line
        if (pass == 0) totalH -= linespace;
    }
    if (w) *w = maxW;
    if (h) *h = totalH;
err:
    if (result) *result = res;
    free(buf);
    return surf;
}

static SDL_Surface* RenderText(Label::FONT font, const char* text, SDL_Color color, Label::HAlign halign)
{
    return _RenderText(font, text, color, halign);
}

static int SizeText(Label::FONT font, const char* text, int* w, int* h)
{
    int res;
    _RenderText(font, text, {0,0,0,0}, Label::HAlign::LEFT, 1, w, h, &res);
    return res;
}


Label::Label(int x, int y, int w, int h, FONT font, const std::string& text)
    : Widget(x,y,w,h), _font(font), _text(text)
{
    int autoW=0, autoH=0;
    if (_font && !_text.empty()) SizeText(_font, _text.c_str(), &autoW, &autoH);
    _autoSize = { autoW, autoH };
    _minSize = _autoSize; // until we support streching or elipsis
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
        SDL_Color color = {_textColor.r, _textColor.g, _textColor.b};
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
    _minSize = _autoSize; // until we support streching or elipsis
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
