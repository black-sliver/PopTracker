#pragma once

#include "widget.h"

namespace Ui {

void drawRect(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC);

void drawRectGlow(Renderer renderer, Position pos, Size size, Widget::Color color);

void drawDiamond(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC);

void drawDiamondGlow(Renderer renderer, Position pos, Size size, Widget::Color color);

void drawTrapezoid(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC);

void drawTrapezoidGlow(Renderer renderer, Position pos, Size size, Widget::Color color);

} // namespace Ui
