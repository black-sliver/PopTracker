#ifndef _UILIB_LABEL_H
#define _UILIB_LABEL_H

#include <SDL_ttf.h>
#include <string>
#include "widget.h"

namespace Ui {

class Label : public Widget
{
public:
    enum class HAlign {
        LEFT = -1,
        CENTER = 0,
        RIGHT = 1
    };
    enum class VAlign {
        TOP = -1,
        MIDDLE = 0,
        BOTTOM = 1
    };
    using FONT = TTF_Font*;
    Label(int x, int y, int w, int h, FONT font, const std::string& text);
    virtual ~Label();
    
    virtual void render(Renderer renderer, int offX, int offY);
    
protected:
    FONT _font;
    std::string _text;
    SDL_Texture *_tex = nullptr;
    Widget::Color _textColor = {255,255,255};
    HAlign _halign = HAlign::CENTER;
    VAlign _valign = VAlign::MIDDLE;
    
public:
    virtual void setText(const std::string& text);
    virtual void setTextAlignment(HAlign halign, VAlign valign) { _halign = halign; _valign = valign; }
    virtual void setTextColor(Widget::Color c);
    const std::string& getText() const { return _text; }
    const Widget::Color getTextColor() const { return _textColor; }
    
};

} // namespace Ui

#endif // _UILIB_LABEL_H
