#pragma once

#include "../core/location.h"
#include "widget.h"

namespace Ui {
    typedef ::Location::MapLocation::Orientation Orientation;

void drawRect(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC);

void drawRectGlow(Renderer renderer, Position pos, Size size, Widget::Color color);

void drawDiamond(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC);

void drawDiamondGlow(Renderer renderer, Position pos, Size size, Widget::Color color);

void drawTrapezoid(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC,
        Orientation orientation);

void drawTrapezoidGlow(Renderer renderer, Position pos, Size size, Widget::Color color,
        Orientation orientation);

void drawTriangle(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC,
        Orientation orientation);

void drawTriangleGlow(Renderer renderer, Position pos, Size size, Widget::Color color,
        Orientation orientation);

void drawConcaveKite(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC,
        Orientation orientation);

void drawConcaveKiteGlow(Renderer renderer, Position pos, Size size, Widget::Color color,
        Orientation orientation);

} // namespace Ui
