#include "group.h"
#include "label.h"

namespace Ui {

constexpr int TITLE_HEIGHT = 24; // 25
constexpr int TITLE_MARGIN = 11;
constexpr SDL_Color TITLE_BG = { 0x21,0x21,0x21,255 };

Group::Group(int x, int y, int w, int h, FONT font, const std::string& title)
    : VBox(x,y,w,h)
{
    _margin = 4;
    _spacing = 4;
    // TODO: have label with background color, padding and gravity instead of overriding render()?
    _lbl = new Label(x+TITLE_MARGIN, y, w-2*TITLE_MARGIN, TITLE_HEIGHT,
                         font, title.c_str());
    if (_lbl->getWidth() < _lbl->getAutoWidth()) {
        _lbl->setWidth(_lbl->getAutoWidth());
    }
    _lbl->setMinSize({_lbl->getMinWidth(), _lbl->getHeight()});
    _lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
    _minSize.width = _lbl->getWidth() + 2*TITLE_MARGIN;
    _minSize.height = TITLE_HEIGHT;
    // this is consistent with VBox::addChild, but keeps TITLE_MARGIN
    setSize({_lbl->getWidth() + 2*TITLE_MARGIN, _lbl->getHeight()}); 
    Container::addChild(_lbl);
}


void Group::render(Renderer renderer, int offX, int offY)
{
    SDL_SetRenderDrawColor(renderer, TITLE_BG.r, TITLE_BG.g, TITLE_BG.b, TITLE_BG.a);
    SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, TITLE_HEIGHT };
    SDL_RenderFillRect(renderer, &r);
    VBox::render(renderer, offX, offY);
}

} // namespace
