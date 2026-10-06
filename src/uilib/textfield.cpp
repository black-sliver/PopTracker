#include "textfield.h"
#include "window.h"
#include "textutil.h"
#include <SDL2/SDL.h>
#include <stdlib.h>

static bool startsCodepoint(unsigned char b)
{
    return (b & 0x80) == 0 || (b & 0xc0) == 0xc0;
}

static size_t prevCodepoint(const std::string& s, size_t i)
{
    size_t j = i - 1;
    while (j > 0 && !startsCodepoint((unsigned char)s[j])) j--;
    return j;
}

static size_t nextCodepoint(const std::string& s, size_t i)
{
    size_t j = i + 1;
    while (j < s.length() && !startsCodepoint((unsigned char)s[j])) j++;
    return j;
}

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
        size_t len = _text.length();
        if (key == SDLK_BACKSPACE) {
            if (_cursor > 0) {
                size_t start = prevCodepoint(_text, _cursor);
                _text.erase(start, _cursor - start);
                _cursor = start;
                onTextChanged.emit(this, _text);
            }
        }
        else if (key == SDLK_DELETE) {
            if (_cursor < len) {
                _text.erase(_cursor, nextCodepoint(_text, _cursor) - _cursor);
                onTextChanged.emit(this, _text);
            }
        }
        else if (key == SDLK_LEFT) {
            if (_cursor > 0) _cursor = prevCodepoint(_text, _cursor);
        }
        else if (key == SDLK_RIGHT) {
            if (_cursor < len) _cursor = nextCodepoint(_text, _cursor);
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
        _cursor += text.length();
        onTextChanged.emit(this, _text);
    }};
}

void TextField::invalidateTexture()
{
    _texture.reset();
    _textureRenderer = nullptr;
    _textureText.clear();
    _textureColor = {0,0,0,0};
    _textureW = _textureH = 0;
}

SDL_Texture* TextField::getTexture(Renderer renderer, const std::string& text, Widget::Color color)
{
    if (_texture && (_textureRenderer != renderer
                    || _textureText != text
                    || _textureColor.r != color.r || _textureColor.g != color.g
                    || _textureColor.b != color.b || _textureColor.a != color.a)) {
        invalidateTexture();
    }
    if (!_texture && _font) {
        SDL_Color col = {color.r, color.g, color.b, color.a};
        SDL_Surface* surf = RenderText(_font, text.c_str(), col, Label::HAlign::LEFT);
        if (surf) {
            _texture.reset(SDL_CreateTextureFromSurface(renderer, surf));
            _textureW = surf->w;
            _textureH = surf->h;
            SDL_FreeSurface(surf);
        }
        if (_texture) {
            _textureRenderer = renderer;
            _textureText = text;
            _textureColor = color;
        }
    }
    return _texture.get();
}

int TextField::getTextWidth(const std::string& text) const
{
    int w = 0;
    if (_font) SizeText(_font, text.c_str(), &w, nullptr);
    return w;
}

void TextField::setCursorToPos(int x)
{
    size_t len = _text.length();
    size_t best = 0;
    // distance from click to cursor position 0, which sits left of all text
    int bestDist = abs(x);
    for (size_t i = nextCodepoint(_text, 0); i <= len; i = nextCodepoint(_text, i)) {
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
    _cursor = _text.length();
    onTextChanged.emit(this, _text);
}

void TextField::setPlaceholder(const std::string& placeholder)
{
    if (_placeholder == placeholder) return;
    _placeholder = placeholder;
}

void TextField::setTextColor(Widget::Color c)
{
    if (_textColor.r == c.r && _textColor.g == c.g && _textColor.b == c.b && _textColor.a == c.a)
        return;
    _textColor = c;
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
        SDL_Texture* tex = getTexture(renderer, text, color);
        if (tex) {
            SDL_Rect dest = { offX+_pos.left+2, offY+_pos.top+(_size.height-_textureH)/2, _textureW, _textureH };
            SDL_Rect src = { 0,0,_textureW,_textureH };
            if (dest.w > _size.width-4) { dest.w = _size.width-4; src.w = dest.w; }
            if (dest.h > _size.height) { dest.h = _size.height; src.h = dest.h; }
            SDL_RenderCopy(renderer, tex, &src, &dest);
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
