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

void drawDiamond(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color tlC, Widget::Color blC, Widget::Color brC, Widget::Color trC)
{
    bool hasAlpha = tlC.a != 0xff || blC.a != 0xff || brC.a != 0xff || trC.a != 0xff;

    float il = pos.left;
    float it = pos.top;
    float iw = size.width;
    float ih = size.height;

    float ol = il - borderWidth;
    float ot = it - borderWidth;
    float ow = iw + 2 * borderWidth;
    float oh = ih + 2 * borderWidth;

    if (hasAlpha) {
        // we have to draw 4 individual lines for border
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        float x1 = ol, x2 = ol + borderWidth, x3 = ol + ow/2, x4 = ol + ow - borderWidth, x5 = ol + ow;
        float y1 = ot, y2 = ot + borderWidth, y3 = ot + oh/2, y4 = ot + oh - borderWidth, y5 = ot + oh;
        SDL_Color borderColor = {0, 0, 0, 255};
        SDL_Vertex verts[] = {
            {{x1, y3}, borderColor, {0, 0}},
            {{x2, y3}, borderColor, {0, 0}},
            {{x3, y2}, borderColor, {0, 0}},
            {{x3, y1}, borderColor, {0, 0}},
            {{x4, y3}, borderColor, {0, 0}},
            {{x5, y3}, borderColor, {0, 0}},
            {{x3, y4}, borderColor, {0, 0}},
            {{x3, y5}, borderColor, {0, 0}},
        };
        int indices[] = {
            0, 1, 2,
            0, 2, 3,
            2, 4, 3,
            3, 4, 5,
            4, 6, 5,
            5, 6, 7,
            1, 7, 6,
            7, 1, 0,
        };
        SDL_RenderGeometry(renderer, nullptr, verts, 8, indices, 24);
    } else {
        // border as bigger background rect
        float x1 = ol, x2 = ol + ow/2, x3 = ol + ow;
        float y1 = ot, y2 = ot + oh/2, y3 = ot + oh;
        SDL_Color borderColor = {0, 0, 0, 255};
        SDL_Vertex verts[] = {
            {{x1, y2}, borderColor, {0, 0}},
            {{x2, y3}, borderColor, {0, 0}},
            {{x2, y1}, borderColor, {0, 0}},
            {{x3, y2}, borderColor, {0, 0}},
        };
        int indices[] = {
            0, 1, 2,
            2, 1, 3
        };
        SDL_RenderGeometry(renderer, nullptr, verts, 4, indices, 6);
    }

    {
        float x1 = il, x2 = il + iw/2, x3 = il + iw;
        float y1 = it, y2 = it + ih/2, y3 = it + ih;
        if (tlC == blC) {
            SDL_Color leftColor = {tlC.r, tlC.g, tlC.b, tlC.a};
            SDL_Vertex verts[] = {
                {{x1, y2}, leftColor, {0, 0}},
                {{x2, y3}, leftColor, {0, 0}},
                {{x2, y1}, leftColor, {0, 0}},
            };
            SDL_RenderGeometry(renderer, nullptr, verts, 3, nullptr, 0);
        } else {
            SDL_Color tlColor = {tlC.r, tlC.g, tlC.b, tlC.a};
            SDL_Color blColor = {blC.r, blC.g, blC.b, blC.a};
            SDL_Vertex verts[] = {
                {{x2, y2}, tlColor, {0, 0}},
                {{x2, y1}, tlColor, {0, 0}},
                {{x1, y2}, tlColor, {0, 0}},
                {{x1, y2}, blColor, {0, 0}},
                {{x2, y3}, blColor, {0, 0}},
                {{x2, y2}, blColor, {0, 0}},
            };
            SDL_RenderGeometry(renderer, nullptr, verts, 6, nullptr, 0);
        }

        if (brC == trC) {
            SDL_Color rightColor = {trC.r, trC.g, trC.b, trC.a};
            SDL_Vertex verts[] = {
                {{x2, y1}, rightColor, {0, 0}},
                {{x2, y3}, rightColor, {0, 0}},
                {{x3, y2}, rightColor, {0, 0}},
            };
            SDL_RenderGeometry(renderer, nullptr, verts, 3, nullptr, 0);
        } else {
            SDL_Color trColor = {trC.r, trC.g, trC.b, trC.a};
            SDL_Color brColor = {brC.r, brC.g, brC.b, brC.a};
            SDL_Vertex verts[] = {
                {{x2, y1}, trColor, {0, 0}},
                {{x2, y2}, trColor, {0, 0}},
                {{x3, y2}, trColor, {0, 0}},
                {{x3, y2}, brColor, {0, 0}},
                {{x2, y2}, brColor, {0, 0}},
                {{x2, y3}, brColor, {0, 0}},
            };
            SDL_RenderGeometry(renderer, nullptr, verts, 6, nullptr, 0);
        }
    }
}

} // namespace Ui
