#include "group.h"
#include "label.h"

namespace Ui {

constexpr int TITLE_HEIGHT = 24; // 25
constexpr int TITLE_MARGIN = 11;
constexpr SDL_Color TITLE_BG = { 0x21,0x21,0x21,255 };

Group::Group(int x, int y, int w, int h, FONT font, const std::string& title)
    : VBox(x,y,w,h)
{
    _padding = 0;
    _spacing = 0;
    // TODO: have label with background color, padding and gravity instead of overriding render()?
    _lbl = new Label(TITLE_MARGIN, 0, 0, TITLE_HEIGHT, font, title.c_str());
    _lbl->setWidth(_lbl->getAutoWidth());
    _lbl->setMinSize(_lbl->getSize());
    _lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
    _lbl->setMargin({TITLE_MARGIN,0,0,0});
    addChild(_lbl);
}


void Group::render(Renderer renderer, int offX, int offY)
{
    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect r = { offX+_pos.left-_margin.left,
                       offY+_pos.top-_margin.top,
                       _size.width+_margin.left+_margin.right,
                       _size.height+_margin.top+_margin.bottom
        };
        SDL_RenderFillRect(renderer, &r);
    }
    SDL_SetRenderDrawColor(renderer, TITLE_BG.r, TITLE_BG.g, TITLE_BG.b, TITLE_BG.a);
    SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, TITLE_HEIGHT };
    SDL_RenderFillRect(renderer, &r);
    for (auto& child: _children)
        if (child->getVisible())
            child->render(renderer, offX+_pos.left, offY+_pos.top);
}

} // namespace
