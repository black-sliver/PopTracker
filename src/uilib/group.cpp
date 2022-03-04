#include "group.h"
#include "label.h"

namespace Ui {

constexpr int TITLE_HEIGHT = 24; // 25
constexpr int TITLE_MARGIN = 11;
constexpr SDL_Color TITLE_BG = { 0x21,0x21,0x21,255 };

Group::Group(int x, int y, int w, int h, FONT font, const std::string& title)
    : VBox(x,y,w,h)
{
    _padding = 4;
    _spacing = 4;
    // TODO: have label with background color, padding and gravity instead of overriding render()?
    _lbl = new Label(TITLE_MARGIN, 0, 0, TITLE_HEIGHT, font, title.c_str());
    _lbl->setWidth(_lbl->getAutoWidth());
    _lbl->setMinSize(_lbl->getSize());
    _lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
    _lbl->setMargin({TITLE_MARGIN,-4,TITLE_MARGIN,0});
    addChild(_lbl);
}


void Group::render(Renderer renderer, int offX, int offY)
{
    SDL_SetRenderDrawColor(renderer, TITLE_BG.r, TITLE_BG.g, TITLE_BG.b, TITLE_BG.a);
    SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, TITLE_HEIGHT };
    SDL_RenderFillRect(renderer, &r);
    VBox::render(renderer, offX, offY);
}

} // namespace
