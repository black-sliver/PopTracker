#include "button.h"

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

void Button::setText(const std::string& text)
{
    Label::setText(text);
    _autoSize.width += 2*_padding;
    _autoSize.height += 2*_padding;
    _minSize.width += 2*_padding;
    _minSize.height += 2*_padding;
}

void Button::render(Renderer renderer, int offX, int offY)
{
    _autoSize.width -= 2*_padding;
    _autoSize.height -= 2*_padding;
    auto originalBgColor = _backgroundColor;
    
    if (getPressed()) {
        _backgroundColor.r/=2;
        _backgroundColor.g/=2;
        _backgroundColor.b/=2;
    }
    
    Label::render(renderer, offX, offY);
    
    // TODO: draw border
    
    _backgroundColor = originalBgColor;
    _autoSize.width += 2*_padding;
    _autoSize.height += 2*_padding;
}

} // namespace

