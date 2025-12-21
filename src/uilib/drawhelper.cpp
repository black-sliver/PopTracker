#include "drawhelper.h"
#include "imghelper.h"
#include "texturemanager.h"
#include "../core/assets.h"

namespace Ui {

static SDL_Texture* getGlowTexture(Renderer renderer)
{
    static fs::path glowAsset = asset("glow64.png");
    SDL_Texture* tex = TextureManager::get(renderer, glowAsset);
    return tex;
}

void drawRect(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color topC, Widget::Color leftC, Widget::Color botC, Widget::Color rightC)
{
    SDL_Rect inner = {
        pos.left, pos.top, size.width, size.height
    };
    SDL_Rect outer = {
        pos.left-borderWidth, pos.top-borderWidth, size.width + 2*borderWidth, size.height + 2*borderWidth
    };
    SDL_Rect outerTop = {
        outer.x, outer.y, outer.w, borderWidth
    };
    SDL_Rect outerBot = {
        outer.x, outer.y + outer.h - borderWidth, outer.w, borderWidth
    };
    SDL_Rect outerLeft = {
        outer.x, outer.y, borderWidth, outer.h
    };
    SDL_Rect outerRight = {
        outer.x + outer.w - borderWidth, outer.y, borderWidth, outer.h
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

void drawTrapezoid(Renderer renderer, Position pos, Size size, int borderWidth,
        Widget::Color tC, Widget::Color lC, Widget::Color bC, Widget::Color rC)
{
    bool hasAlpha = tC.a != 0xff || lC.a != 0xff || bC.a != 0xff || rC.a != 0xff;

    float il = pos.left;
    float it = pos.top;
    float iw = size.width;
    float ih = size.height;

    float ol = il - borderWidth;
    float ot = it - borderWidth;
    float ow = iw + 2 * borderWidth;
    float oh = ih + 2 * borderWidth;
    SDL_Color borderColor = {0, 0, 0, 255};

    if (hasAlpha) {
        // we have to draw 4 individual lines for border
        float x1 = ol, x2 = ol + borderWidth, x3 = ol + ow/4, x4 = ol + ow/4 + borderWidth;
        float x5 = ol + 3*ow/4 - borderWidth, x6 = ol + 3*ow/4, x7 = ol + ow - borderWidth, x8 = ol + ow;
        float y1 = ot, y2 = ot + borderWidth, y3 = ot + oh - borderWidth, y4 = ot + oh;
        SDL_Vertex verts[] = {
            {{x1, y4}, borderColor, {0, 0}},
            {{x2, y3}, borderColor, {0, 0}},
            {{x3, y1}, borderColor, {0, 0}},
            {{x4, y2}, borderColor, {0, 0}},
            {{x5, y2}, borderColor, {0, 0}},
            {{x6, y1}, borderColor, {0, 0}},
            {{x7, y3}, borderColor, {0, 0}},
            {{x8, y4}, borderColor, {0, 0}},
        };
        int indices[] = {
            0, 1, 2,
            2, 1, 3,
            2, 3, 4,
            2, 4, 5,
            5, 4, 6,
            5, 6, 7,
            7, 1, 6,
            7, 0, 1,
        };
        SDL_RenderGeometry(renderer, nullptr, verts, 8, indices, 24);
    } else {
        // border as bigger background shape
        float x1 = ol, x3 = ol + ow/4, x6 = ol + 3*ow/4, x8 = ol + ow;
        float y1 = ot, y4 = ot + oh;
        SDL_Vertex verts[] = {
            {{x1, y4}, borderColor, {0, 0}},
            {{x3, y1}, borderColor, {0, 0}},
            {{x6, y1}, borderColor, {0, 0}},
            {{x8, y4}, borderColor, {0, 0}},
        };
        int indices[] = {
            0, 3, 1,
            1, 3, 2
        };
        SDL_RenderGeometry(renderer, nullptr, verts, 4, indices, 6);
    }

    {
        float x1 = il, x2 = ol + ow/4 + borderWidth, x3 = il + iw/2, x4 = ol + 3*ow/4 - borderWidth, x5 = il + iw;
        float y1 = it, y2 = it + ih/2, y3 = it + ih;

        SDL_Color tColor = {tC.r, tC.g, tC.b, tC.a};
        SDL_Color lColor = {lC.r, lC.g, lC.b, lC.a};
        SDL_Color bColor = {bC.r, bC.g, bC.b, bC.a};
        SDL_Color rColor = {rC.r, rC.g, rC.b, rC.a};
        SDL_Vertex verts[] = {
            {{x2, y1}, tColor, {0, 0}},
            {{x3, y2}, tColor, {0, 0}},
            {{x4, y1}, tColor, {0, 0}},
            {{x1, y3}, lColor, {0, 0}},
            {{x3, y2}, lColor, {0, 0}},
            {{x2, y1}, lColor, {0, 0}},
            {{x1, y3}, bColor, {0, 0}},
            {{x5, y3}, bColor, {0, 0}},
            {{x3, y2}, bColor, {0, 0}},
            {{x5, y3}, rColor, {0, 0}},
            {{x4, y1}, rColor, {0, 0}},
            {{x3, y2}, rColor, {0, 0}},
        };
        SDL_RenderGeometry(renderer, nullptr, verts, 12, nullptr, 0);
    }
}

void drawRectGlow(Renderer renderer, const Position pos, const Size size, const Widget::Color color)
{
    constexpr int glowSize = 10;
    const auto x1 = static_cast<float>(pos.left - glowSize);
    const auto x2 = static_cast<float>(pos.left);
    const auto x3 = static_cast<float>(pos.left + size.width);
    const auto x4 = static_cast<float>(pos.left + size.width + glowSize);
    const auto y1 = static_cast<float>(pos.top - glowSize);
    const auto y2 = static_cast<float>(pos.top);
    const auto y3 = static_cast<float>(pos.top + size.height);
    const auto y4 = static_cast<float>(pos.top + size.height + glowSize);

    const SDL_Color c = {color.r, color.g, color.b, color.a};
    const SDL_Vertex verts[] = {
        {{x1, y1}, c, {0, 0}},
        {{x2, y2}, c, {0.49, 0.49}},
        {{x2, y1}, c, {0.49, 0}},
        {{x1, y2}, c, {0, 0.49}},

        {{x3, y1}, c, {0.51, 0}},
        {{x4, y2}, c, {1, 0.49}},
        {{x4, y1}, c, {1, 0}},
        {{x3, y2}, c, {0.51, 0.49}},

        {{x3, y3}, c, {0.51, 0.51}},
        {{x4, y4}, c, {1, 1}},
        {{x4, y3}, c, {1, 0.51}},
        {{x3, y4}, c, {0.51, 1}},

        {{x1, y3}, c, {0, 0.51}},
        {{x2, y4}, c, {0.49, 1}},
        {{x2, y3}, c, {0.49, 0.51}},
        {{x1, y4}, c, {0, 1}},
    };

    const int indices[] = {
        0, 1, 2,
        0, 3, 1,

        1, 7, 2,
        2, 7, 4,

        4, 7, 5,
        5, 6, 4,

        7, 8, 10,
        10, 5, 7,

        8, 11, 9,
        9, 10, 8,

        8, 14, 11,
        11, 14, 13,

        13, 14, 12,
        12, 15, 13,

        14, 1, 3,
        3, 12, 14,
    };

    const auto tex = getGlowTexture(renderer);
    if (!tex)
        return;
    SDL_RenderGeometry(renderer, tex, verts, std::size(verts), indices, std::size(indices));
}

void drawDiamondGlow(Renderer renderer, const Position pos, const Size size, const Widget::Color color)
{
    constexpr float sqrtOf2 = 1.41421356237;
    constexpr int glowSize = 10;
    constexpr float glowDiag = glowSize * sqrtOf2;
    const float x1 = static_cast<float>(pos.left) - glowDiag;
    const float x2 = static_cast<float>(pos.left) - glowDiag / 2;
    const float x3 = static_cast<float>(pos.left); // NOLINT(*-use-auto)
    const float x4 = static_cast<float>(pos.left) + static_cast<float>(size.width) / 2 - glowDiag / 2;
    const float x5 = static_cast<float>(pos.left) + static_cast<float>(size.width) / 2;
    const float x6 = static_cast<float>(pos.left) + static_cast<float>(size.width) / 2 + glowDiag / 2;
    const float x7 = static_cast<float>(pos.left) + static_cast<float>(size.width);
    const float x8 = static_cast<float>(pos.left) + static_cast<float>(size.width) + glowDiag / 2;
    const float x9 = static_cast<float>(pos.left) + static_cast<float>(size.width) + glowDiag;
    const float y1 = static_cast<float>(pos.top) - glowDiag;
    const float y2 = static_cast<float>(pos.top) - glowDiag / 2;
    const float y3 = static_cast<float>(pos.top); // NOLINT(*-use-auto)
    const float y4 = static_cast<float>(pos.top) + static_cast<float>(size.height) / 2 - glowDiag / 2;
    const float y5 = static_cast<float>(pos.top) + static_cast<float>(size.height) / 2;
    const float y6 = static_cast<float>(pos.top) + static_cast<float>(size.height) / 2 + glowDiag / 2;
    const float y7 = static_cast<float>(pos.top) + static_cast<float>(size.height);
    const float y8 = static_cast<float>(pos.top) + static_cast<float>(size.height) + glowDiag / 2;
    const float y9 = static_cast<float>(pos.top) + static_cast<float>(size.height) + glowDiag;

    const SDL_Color c = {color.r, color.g, color.b, color.a};
    const SDL_Vertex verts[] = {
        {{x5, y1}, c, {0, 0}},
        {{x5, y3}, c, {0.5, 0.5}},
        {{x6, y2}, c, {0.5, 0}},
        {{x4, y2}, c, {0, 0.5}},

        {{x8, y4}, c, {0.5, 0}},
        {{x8, y6}, c, {1, 0.5}},
        {{x9, y5}, c, {1, 0}},
        {{x7, y5}, c, {0.5, 0.5}},

        {{x5, y7}, c, {0.5, 0.5}},
        {{x5, y9}, c, {1, 1}},
        {{x6, y8}, c, {1, 0.5}},
        {{x4, y8}, c, {0.5, 1}},

        {{x2, y4}, c, {0, 0.5}},
        {{x2, y6}, c, {0.5, 1}},
        {{x3, y5}, c, {0.5, 0.5}},
        {{x1, y5}, c, {0, 1}},
    };

    const int indices[] = {
        0, 1, 2,
        0, 3, 1,

        1, 7, 2,
        2, 7, 4,

        4, 7, 5,
        5, 6, 4,

        7, 8, 10,
        10, 5, 7,

        8, 11, 9,
        9, 10, 8,

        8, 14, 11,
        11, 14, 13,

        13, 14, 12,
        12, 15, 13,

        14, 1, 3,
        3, 12, 14,
    };

    const auto tex = getGlowTexture(renderer);
    if (!tex)
        return;
    SDL_RenderGeometry(renderer, tex, verts, std::size(verts), indices, std::size(indices));
}

void drawTrapezoidGlow(Renderer renderer, const Position pos, const Size size, const Widget::Color color)
{
    constexpr int glowSize = 10;
    const float left2 = static_cast<float>(pos.left) + static_cast<float>(size.width) / 4;
    const float width2 = static_cast<float>(size.width) / 2;
    constexpr int glowOff1 = 12;
    constexpr int glowOff2 = 14;
    constexpr int glowOff3 = 6;
    const float x1  = static_cast<float>(pos.left) - glowOff2;
    const float x2  = static_cast<float>(pos.left) - glowOff1;
    const float x3  = static_cast<float>(pos.left); // NOLINT(*-use-auto)
    const float x4  = left2 - glowOff1;
    const float x5  = left2 - glowOff3;
    const float x6  = left2;
    const float x7  = left2 + width2;
    const float x8  = left2 + width2 + glowOff3;
    const float x9  = left2 + width2 + glowOff1;
    const float x10 = static_cast<float>(pos.left + size.width); // NOLINT(*-use-auto)
    const float x11 = static_cast<float>(pos.left + size.width) + glowOff1;
    const float x12 = static_cast<float>(pos.left + size.width) + glowOff2;
    const float y1  = static_cast<float>(pos.top) - glowSize;
    const float y2  = static_cast<float>(pos.top); // NOLINT(*-use-auto)
    const float y3  = static_cast<float>(pos.top + size.height); // NOLINT(*-use-auto)
    const float y4  = static_cast<float>(pos.top + size.height) + glowSize;

    const SDL_Color c = {color.r, color.g, color.b, color.a};
    const SDL_Vertex verts[] = {
        {{x5, y1}, c, {0, 0}},
        {{x6, y2}, c, {0.49, 0.49}},
        {{x6, y1}, c, {0.49, 0}},
        {{x4, y2}, c, {0, 0.49}},

        {{x7, y1}, c, {0.51, 0}},
        {{x9, y2}, c, {1, 0.49}},
        {{x8, y1}, c, {1, 0}},
        {{x7, y2}, c, {0.51, 0.49}},

        {{x10, y3}, c, {0.51, 0.51}},
        {{x12, y4}, c, {1, 1}},
        {{x11, y3}, c, {1, 0.51}},
        {{x10, y4}, c, {0.51, 1}},

        {{x2, y3}, c, {0, 0.51}},
        {{x3, y4}, c, {0.49, 1}},
        {{x3, y3}, c, {0.49, 0.51}},
        {{x1, y4}, c, {0, 1}},
    };

    const int indices[] = {
        2, 1, 3,
        3, 0, 2,

        1, 7, 2,
        2, 7, 4,

        4, 7, 5,
        5, 6, 4,

        7, 8, 10,
        10, 5, 7,

        8, 11, 9,
        9, 10, 8,

        8, 14, 11,
        11, 14, 13,

        13, 14, 12,
        12, 15, 13,

        14, 1, 3,
        3, 12, 14,
    };

    const auto tex = getGlowTexture(renderer);
    if (!tex)
        return;
    SDL_RenderGeometry(renderer, tex, verts, std::size(verts), indices, std::size(indices));
}

} // namespace Ui
