#pragma once

#include <SDL2/SDL_ttf.h>
#include <memory>
#include <string>
#include "widget.h"
#include "texturemanager.h"

namespace Ui {

class Window;

class TextField : public Widget {
public:
    using FONT = TTF_Font*;
    TextField(int x, int y, int w, int h, FONT font, Window *window=nullptr);

    virtual void render(Renderer renderer, int offX, int offY) override;

    virtual void setText(const std::string& text);
    const std::string& getText() const { return _text; }
    void clear();
    virtual void setPlaceholder(const std::string& placeholder);
    virtual void setTextColor(Widget::Color c);
    virtual void setBackground(Widget::Color color) override { _backgroundColor = color; }

    void grabFocus();
    void releaseFocus();

    Signal<const std::string&> onTextChanged;

protected:
    FONT _font;
    std::string _text;
    std::string _placeholder;
    size_t _cursor = 0;
    Window *_window = nullptr;
    Widget::Color _textColor = {255,255,255};
    Widget::Color _placeholderColor = {128,128,128};

    std::unique_ptr<SDL_Texture, SDLTextureDeleter> _texture;
    SDL_Renderer *_textureRenderer = nullptr;
    std::string _textureText;
    Widget::Color _textureColor = {0,0,0,0};
    int _textureW = 0;
    int _textureH = 0;

    void invalidateTexture();
    SDL_Texture* getTexture(Renderer renderer, const std::string& text, Widget::Color color);

    int getTextWidth(const std::string& text) const;
    void setCursorToPos(int x);
};

} // namespace Ui
