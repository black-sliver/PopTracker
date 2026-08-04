#ifndef _UILIB_TEXTFIELD_H
#define _UILIB_TEXTFIELD_H

#include <SDL2/SDL_ttf.h>
#include <string>
#include "widget.h"

namespace Ui {

class Window;

class TextField : public Widget {
public:
    using FONT = TTF_Font*;
    TextField(int x, int y, int w, int h, FONT font, Window *window=nullptr);
    virtual ~TextField();

    virtual void render(Renderer renderer, int offX, int offY) override;

    virtual void setText(const std::string& text);
    const std::string& getText() const { return _text; }
    void clear();
    void setPlaceholder(const std::string& placeholder) { _placeholder = placeholder; }
    virtual void setTextColor(Widget::Color c) { _textColor = c; }
    virtual void setBackground(Widget::Color color) override { _backgroundColor = color; }

    void grabFocus();
    void releaseFocus();

    Signal<const std::string&> onTextChanged;

protected:
    FONT _font;
    std::string _text;
    std::string _placeholder;
    int _cursor = 0;
    Window *_window = nullptr;
    Widget::Color _textColor = {255,255,255};
    Widget::Color _placeholderColor = {128,128,128};

    int getTextWidth(const std::string& text) const;
    void setCursorToPos(int x);
};

} // namespace Ui

#endif // _UILIB_TEXTFIELD_H
