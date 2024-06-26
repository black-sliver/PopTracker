#pragma once

#include "widget.h"

namespace Ui {

void drawRect(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC);

void drawDiamond(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC);


} // namespace Ui
