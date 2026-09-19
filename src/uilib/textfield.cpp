#include "textfield.h"
#include "window.h"
#include "textutil.h"
#include <SDL2/SDL.h>
#include <stdlib.h>

namespace Ui {

TextField::TextField(int x, int y, int w, int h, FONT font, Window *window)
    : Widget(x,y,w,h), _font(font), _window(window)
{
    _backgroundColor = {32,32,32};
    if (_font && h <= 0) {
        int textH = 0;
        SizeText(_font, " ", nullptr, &textH);
        h = textH + 6;
    }
    if (h > 0) setHeight(h);
    setMinSize({0, getHeight()});

    onClick += {this, [this](void*, int x, int, int button) {
        if (button == MouseButton::BUTTON_LEFT) {
            grabFocus();
            setCursorToPos(x);
        }
    }};

    onKeyDown += {this, [this](void*, int key, int mod) {
        (void)mod;
        int len = (int)_text.length();
        if (key == SDLK_BACKSPACE) {
            if (_cursor > 0) {
                _text.erase(_cursor-1, 1);
                _cursor--;
                onTextChanged.emit(this, _text);
            }
        }
        else if (key == SDLK_DELETE) {
            if (_cursor < len) {
                _text.erase(_cursor, 1);
                onTextChanged.emit(this, _text);
            }
        }
        else if (key == SDLK_LEFT) {
            if (_cursor > 0) _cursor--;
        }
        else if (key == SDLK_RIGHT) {
            if (_cursor < len) _cursor++;
        }
        else if (key == SDLK_HOME) {
            _cursor = 0;
        }
        else if (key == SDLK_END) {
            _cursor = len;
        }
        else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            releaseFocus();
        }
        else if (key == SDLK_ESCAPE) {
            clear();
            releaseFocus();
        }
    }};

    onTextInput += {this, [this](void*, const std::string& text) {
        _text.insert(_cursor, text);
        _cursor += (int)text.length();
        onTextChanged.emit(this, _text);
    }};
}

TextField::~TextField()
{
}

int TextField::getTextWidth(const std::string& text) const
{
    int w = 0;
    if (_font) SizeText(_font, text.c_str(), &w, nullptr);
    return w;
}

void TextField::setCursorToPos(int x)
{
    int len = (int)_text.length();
    int best = 0;
    int bestDist = abs(x - getTextWidth(_text));
    for (int i=1; i<=len; i++) {
        int dist = abs(x - getTextWidth(_text.substr(0, i)));
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    _cursor = best;
}

void TextField::setText(const std::string& text)
{
    if (_text == text) return;
    _text = text;
    _cursor = (int)_text.length();
    onTextChanged.emit(this, _text);
}

void TextField::clear()
{
    setText("");
}

void TextField::grabFocus()
{
    if (_window) _window->setKeyboardFocus(this);
}

void TextField::releaseFocus()
{
    if (_window) _window->setKeyboardFocus(nullptr);
}

void TextField::render(Renderer renderer, int offX, int offY)
{
    const auto& c = _backgroundColor;
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
    SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, _size.height };
    SDL_RenderFillRect(renderer, &r);

    const std::string& text = _text.empty() ? _placeholder : _text;
    Widget::Color color = _text.empty() ? _placeholderColor : _textColor;
    if (!text.empty() && _font) {
        SDL_Color col = {color.r, color.g, color.b, color.a};
        SDL_Surface* surf = RenderText(_font, text.c_str(), col, Label::HAlign::LEFT);
        if (surf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
            SDL_Rect dest = { offX+_pos.left+2, offY+_pos.top+(_size.height-surf->h)/2, surf->w, surf->h };
            SDL_Rect src = { 0,0,surf->w,surf->h };
            if (dest.w > _size.width-4) { dest.w = _size.width-4; src.w = dest.w; }
            if (dest.h > _size.height) { dest.h = _size.height; src.h = dest.h; }
            SDL_RenderCopy(renderer, tex, &src, &dest);
            SDL_DestroyTexture(tex);
            SDL_FreeSurface(surf);
        }
    }

    if (_window && _window->getKeyboardFocus() == this && (SDL_GetTicks()/500)%2==0) {
        int caretX = 2 + getTextWidth(_text.substr(0, _cursor));
        int caretH = _size.height > 4 ? _size.height-4 : 1;
        SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
        SDL_Rect caret = { offX+_pos.left+caretX, offY+_pos.top+2, 1, caretH };
        SDL_RenderFillRect(renderer, &caret);
    }
}

} // namespace
