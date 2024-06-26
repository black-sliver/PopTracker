#include "drawhelper.h"

namespace Ui {

void drawRect(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC)
{
    SDL_Rect inner = {
        .x = pos.left, .y = pos.top, .w = size.width, .h = size.height
    };
    SDL_Rect outer = {
        .x = pos.left-borderWidth, .y = pos.top-borderWidth, .w = size.width + 2*borderWidth, .h = size.height + 2*borderWidth
    };
    SDL_Rect outerTop = {
        .x = outer.x, .y = outer.y, .w = outer.w, .h = borderWidth
    };
    SDL_Rect outerBot = {
        .x = outer.x, .y = outer.y + outer.h - borderWidth, .w = outer.w, .h = borderWidth
    };
    SDL_Rect outerLeft = {
        .x = outer.x, .y = outer.y, .w = borderWidth, .h = outer.h
    };
    SDL_Rect outerRight = {
        .x = outer.x + outer.w - borderWidth, .y = outer.y, .w = borderWidth, .h = outer.h
    };

    bool hasAlpha = topC.a != 0xff || leftC.a != 0xff || botC.a != 0xff || rightC.a != 0xff;

    float fx = pos.left;
    float fy = pos.top;
    float fw = size.width;
    float fh = size.height;

    if (hasAlpha) {
        // we have to draw 4 individual lines for border
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &outerTop);
        SDL_RenderFillRect(renderer, &outerBot);
        SDL_RenderFillRect(renderer, &outerLeft);
        SDL_RenderFillRect(renderer, &outerRight);
    } else {
        // border as bigger background rect
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &outer);
    }

    if (!hasAlpha || (topC == leftC && leftC == botC && botC == rightC)) {
        SDL_SetRenderDrawColor(renderer, botC.r, botC.g, botC.b, botC.a);
        SDL_RenderFillRect(renderer, &inner);
    } else if (botC == rightC) {
        SDL_Color botRightColor = {botC.r, botC.g, botC.b, botC.a};
        SDL_Vertex botRightVerts[] = {
            {{fx + fw, fy}, botRightColor, {0, 0}},
            {{fx, fy + fh}, botRightColor, {0, 0}},
            {{fx + fw, fy + fh}, botRightColor, {0, 0}},
        };
        SDL_RenderGeometry(renderer, nullptr, botRightVerts, 3, nullptr, 0);
    } else {
        SDL_Color botColor = {botC.r, botC.g, botC.b, botC.a};
        SDL_Vertex botVerts[] = {
            {{fx, fy + fh}, botColor, {0, 0}},
            {{fx + fw, fy + fw}, botColor, {0, 0}},
            {{fx + fw/2, fy + fh/2}, botColor, {0, 0}},
        };
        SDL_RenderGeometry(renderer, nullptr, botVerts, 3, nullptr, 0);
    }

    if (botC != rightC) {
        SDL_Color rightColor = {rightC.r, rightC.g, rightC.b, rightC.a};
        SDL_Vertex rightVerts[] = {
            {{fx + fw, fy}, rightColor, {0, 0}},
            {{fx + fw/2, fy + fh/2}, rightColor, {0, 0}},
            {{fx + fw, fy + fh}, rightColor, {0, 0}},
        };
        SDL_RenderGeometry(renderer, nullptr, rightVerts, 3, nullptr, 0);
    }

    if (topC == leftC && topC != botC) {
        SDL_Color topLeftColor = {topC.r, topC.g, topC.b, topC.a};
        SDL_Vertex topLeftVerts[] = {
            {{fx, fy}, topLeftColor, {0, 0}},
            {{fx, fy + fh}, topLeftColor, {0, 0}},
            {{fx + fw, fy}, topLeftColor, {0, 0}},
        };
        SDL_RenderGeometry(renderer, nullptr, topLeftVerts, 3, nullptr, 0);
    }

    if (topC != leftC && topC != botC) {
        SDL_Color topColor = {topC.r, topC.g, topC.b, topC.a};
        SDL_Vertex topVerts[] = {
            {{fx, fy}, topColor, {0, 0}},
            {{fx + fw/2, fy + fh/2}, topColor, {0, 0}},
            {{fx + fw, fy}, topColor, {0, 0}},
        };
        SDL_RenderGeometry(renderer, nullptr, topVerts, 3, nullptr, 0);
    }

    if (leftC != topC && leftC != botC) {
        SDL_Color leftColor = {leftC.r, leftC.g, leftC.b, leftC.a};
        SDL_Vertex leftVerts[] = {
            {{fx, fy}, leftColor, {0, 0}},
            {{fx, fy + fh}, leftColor, {0, 0}},
            {{fx + fw/2, fy + fh/2}, leftColor, {0, 0}},
        };
        SDL_RenderGeometry(renderer, nullptr, leftVerts, 3, nullptr, 0);
    }
}

} // namespace Ui
