#include "mapwidget.h"
#include "../core/util.h" // countOf

namespace Ui {

#define _DEFAULT_STATE_COLORS { \
    /* done */ \
    { 0x3f, 0x3f, 0x3f }, \
    /* available -> green */ \
    { 0x20, 0xff, 0x20 }, \
    /* unavailable -> darkish red */ \
    { 0xcf, 0x10, 0x10 }, \
    /* mixed available/unavailable -> light orange */ \
    { 0xff, 0x9f, 0x20 }, \
    /* glitches required -> yellow */ \
    { 0xff, 0xff, 0x20 }, \
    /* mixed available + glitches -> greenish yellow */ \
    { 0xaf, 0xff, 0x20 }, \
    /* mixed unavailable + glitches -> dark orange */ \
    { 0xef, 0x55, 0x00 }, \
    /* mixed all three -> light orange */ \
    { 0xff, 0x9f, 0x20 }, \
\
    /* checkable -> blue */ \
    { 0x30, 0x40, 0xff }, \
    /* checkable + available -> teal */ \
    { 0x20, 0xff, 0xff }, \
    /* checkable + unavailable -> purple */ \
    { 0xc0, 0x10, 0xff }, \
\
    /* mixed checkable + available/unavailable -> light orange */ \
    { 0xff, 0x9f, 0x20 }, \
    /* mixed checkable + glitches required -> darker teal */ \
    { 0x20, 0xd0, 0xd0 }, \
    /* mixed checkable + available + glitches -> light orange */ \
    { 0xff, 0x9f, 0x20 }, \
    /* mixed checkable + unavailable + glitches -> darker teal */ \
    { 0x20, 0xd0, 0xd0 }, \
\
    /* mixed all 4 -> light orange */ \
    { 0xff, 0x9f, 0x20 }, \
\
    /* error */ \
    { 0x00, 0x00, 0x00, 0xff }, \
}

// map 4 state bits to 4 individual triangle colors
// triangle order: top, left, bottom, right
static const int triangleValues[16][4] = {
    {0, 0, 0, 0}, // all done
    {1, 1, 1, 1}, // all reachable
    {2, 2, 2, 2}, // all unreachable
    {2, 2, 1, 1}, // unreachable + reachable
    {4, 4, 4, 4}, // all glitched
    {4, 4, 1, 1}, // glitched + reachable
    {2, 2, 4, 4}, // glitched + unreachable
    {4, 2, 1, 1}, // glitched + reachable + unreachable
    {8, 8, 8, 8}, // all scout
    {8, 8, 1, 1}, // scout + reachable
    {2, 2, 8, 8}, // scout + unreachable
    {8, 2, 1, 1}, // scout + unreachable + reachable
    {4, 4, 8, 8}, // scout + glitched
    {4, 8, 1, 1}, // scout + glitched + reachable
    {4, 2, 8, 8}, // scout + glitched + unreachable
    {4, 2, 1, 8}, // scout + glitched + reachable + unreachable
};


Widget::Color MapWidget::StateColors[] = _DEFAULT_STATE_COLORS;

bool MapWidget::SplitRects = true;


MapWidget::MapWidget(int x, int y, int w, int h, const char* path)
    : Image(x, y, w, h, path)
{
    connectSignals();
}

MapWidget::MapWidget(int x, int y, int w, int h, const void* data, size_t len)
    : Image(x, y, w, h, data, len)
{
    connectSignals();
}

void MapWidget::calculateSizes(int left, int top, int& srcw, int& srch, int& dstx, int& dsty, int& dstw, int& dsth)
{
    srcw = _autoSize.width;
    srch = _autoSize.height;
    if (_fixedAspect) {
        float ar = (float)_autoSize.width / (float)_autoSize.height;
        dstw = (int)(_size.height * ar + 0.5);
        dsth = _size.height;
        if (dstw > _size.width) {
            dsth = (int)(_size.width / ar + 0.5);
            dstw = _size.width;
        }
        dstx = left + (_size.width-dstw+1)/2;
        dsty = top  + (_size.height-dsth+1)/2;
    } else {
        dstx = left;
        dsty = top;
        dstw = _size.width;
        dsth = _size.height;
    }
}

void MapWidget::connectSignals()
{
    this->onMouseMove += { this, [this](void* s, int x, int y, unsigned buttons) {
        int absX = _absX + x;
        int absY = _absY + y;
        int srcw, srch, dstx, dsty, dstw, dsth;
        calculateSizes(0, 0, srcw, srch, dstx, dsty, dstw, dsth);
        int x1 = x-dstx;
        int y1 = y-dsty;
        x1 = (x1*srcw + dstw/2) / dstw;
        y1 = (y1*srch + dsth/2) / dsth;
        bool match = false;
        for (auto locIt = _locations.rbegin(); locIt!=_locations.rend(); locIt++) {
            const auto& loc = locIt->second;
            for (const auto& pos: loc.pos) {
                if (pos.state == -1) continue; // hidden
                if (pos.state == 0 && _hideClearedLocations) continue;
                if (pos.state == 2 && _hideUnreachableLocations) continue;
                int locsize = pos.size + 2 * pos.borderThickness; // or without border?
                int locleft = pos.x - locsize/2;
                int loctop = pos.y - locsize/2;
                if (locleft < 0) locleft = 0;
                if (loctop < 0) loctop = 0;
                if (locleft > srcw-locsize) locleft=srcw-locsize;
                if (loctop > srch-locsize) loctop=srch-locsize;
                if (x1 >= locleft && x1 < locleft+locsize &&
                    y1 >= loctop  && y1 < loctop+locsize)
                {
                    // TODO; store iterator instead of string?
                    match = true;
                    if (locIt->first != _locationHover) {
                        _locationHover = locIt->first;
    #if 0
                        printf("MapWidget: hover location %s\n", _locationHover.c_str());
    #endif
                        onLocationHover.emit(this, _locationHover, absX, absY);
                    }
                    break;
                }
            }
        }
        if (!match) {
            _locationHover = "";
        }
    }};
    this->onMouseLeave += { this, [this](void* s) {
        _locationHover = "";
    }};
}

void MapWidget::render(Renderer renderer, int offX, int offY)
{
    _absX = offX+_pos.left; // add this to relative mouse coordinates to get absolute
    _absY = offY+_pos.top; // FIXME: we should provide absolute AND relative mouse position through the Event stack
    Image::render(renderer, offX, offY);
    
    int srcw, srch, dstx, dsty, dstw, dsth;
    calculateSizes(offX+_pos.left, offY+_pos.top, srcw, srch, dstx, dsty, dstw, dsth);
    
    for (const auto& pair : _locations) {
        const auto& loc = pair.second;
        for (const auto& pos : loc.pos) {
            // location icon size in screen space
            int locScreenInnerW  = (pos.size*dstw+srcw/2)/srcw;
            int locScreenInnerH  = (pos.size*dsth+srch/2)/srch;
            if (locScreenInnerW<1) locScreenInnerW=1;
            if (locScreenInnerH<1) locScreenInnerH=1;
            int borderScreenSize = (pos.borderThickness*dstw+srcw/2)/srcw;
            if (borderScreenSize<1 && pos.borderThickness>0) borderScreenSize=1;
            int locScreenOuterW  = locScreenInnerW+2*borderScreenSize;
            int locScreenOuterH  = locScreenInnerH+2*borderScreenSize;
            // calculate top left corner of squares
            int innerx = (pos.x*dstw+srcw/2)/srcw - locScreenInnerW/2;
            int innery = (pos.y*dsth+srch/2)/srch - locScreenInnerH/2;
            // fix up locations that are on the edge of the map -- we could also move this to addLocation
            if (innerx < borderScreenSize) innerx = borderScreenSize;
            if (innery < borderScreenSize) innery = borderScreenSize;
            if (innerx > dstw+locScreenOuterW) innerx = dstw-locScreenOuterW;
            if (innery > dsth+locScreenOuterH) innery = dsth-locScreenOuterH;
            // move to drawing offset
            innerx += dstx;
            innery += dsty;
            // calculate top left corner of border
            int outerx = innerx-borderScreenSize;
            int outery = innery-borderScreenSize;

            SDL_Rect inner = {
                .x = innerx, .y = innery, .w = locScreenInnerW, .h = locScreenInnerH
            };
            SDL_Rect outer = {
                .x = outerx, .y = outery, .w = locScreenOuterW, .h = locScreenOuterH
            };
            SDL_Rect outerTop = {
                .x = outer.x, .y = outer.y, .w = outer.w, .h = borderScreenSize
            };
            SDL_Rect outerBot = {
                .x = outer.x, .y = outer.y + outer.h - borderScreenSize, .w = outer.w, .h = borderScreenSize
            };
            SDL_Rect outerLeft = {
                .x = outer.x, .y = outer.y, .w = borderScreenSize, .h = outer.h
            };
            SDL_Rect outerRight = {
                .x = outer.x + outer.w - borderScreenSize, .y = outer.y, .w = borderScreenSize, .h = outer.h
            };

            int state = (int)pos.state;
            if (state == -1) continue; // hidden
            if (state == 0 && _hideClearedLocations) continue;
            if (state == 2 && _hideUnreachableLocations) continue;

            bool hasAlpha;

            if (!SplitRects || state<0 || state>=countOf(triangleValues)) {
                const Widget::Color& c = (state<0 || state>=countOf(StateColors)) ?
                        StateColors[countOf(StateColors)-1] : StateColors[state];
                hasAlpha = c.a < 0xff;
                if (hasAlpha) {
                    // we have to draw 4 individual lines for border
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderFillRect(renderer, &outerTop);
                    SDL_RenderFillRect(renderer, &outerBot);
                    SDL_RenderFillRect(renderer, &outerLeft);
                    SDL_RenderFillRect(renderer, &outerRight);
                } else {
                    // border as bigger background rect (optimized for sw rendering)
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                    SDL_RenderFillRect(renderer, &outer);
                }
                SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
                SDL_RenderFillRect(renderer, &inner);

                continue;
            }

            const int* values = triangleValues[state];
            const Widget::Color& topC = StateColors[values[0]];
            const Widget::Color& leftC = StateColors[values[1]];
            const Widget::Color& botC = StateColors[values[2]];
            const Widget::Color& rightC = StateColors[values[3]];

            hasAlpha = (topC.a < 0xff) ||
                       (leftC.a < 0xff) ||
                       (botC.a < 0xff) ||
                       (rightC.a < 0xff);

            float fx = innerx;
            float fy = innery;
            float fw = locScreenInnerW;
            float fh = locScreenInnerH;

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

            if (!hasAlpha || (values[0] == values[1] && values[1] == values[2] && values[2] == values[3])) {
                SDL_SetRenderDrawColor(renderer, botC.r, botC.g, botC.b, botC.a);
                SDL_RenderFillRect(renderer, &inner);
            } else if (values[2] == values[3]) {
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

            if (values[2] != values[3]) {
                SDL_Color rightColor = {rightC.r, rightC.g, rightC.b, rightC.a};
                SDL_Vertex rightVerts[] = {
                    {{fx + fw, fy}, rightColor, {0, 0}},
                    {{fx + fw/2, fy + fh/2}, rightColor, {0, 0}},
                    {{fx + fw, fy + fh}, rightColor, {0, 0}},
                };
                SDL_RenderGeometry(renderer, nullptr, rightVerts, 3, nullptr, 0);
            }

            if (values[0] == values[1] && values[0] != values[2]) {
                SDL_Color topLeftColor = {topC.r, topC.g, topC.b, topC.a};
                SDL_Vertex topLeftVerts[] = {
                    {{fx, fy}, topLeftColor, {0, 0}},
                    {{fx, fy + fh}, topLeftColor, {0, 0}},
                    {{fx + fw, fy}, topLeftColor, {0, 0}},
                };
                SDL_RenderGeometry(renderer, nullptr, topLeftVerts, 3, nullptr, 0);
            }

            if (values[0] != values[1] && values[0] != values[2]) {
                SDL_Color topColor = {topC.r, topC.g, topC.b, topC.a};
                SDL_Vertex topVerts[] = {
                    {{fx, fy}, topColor, {0, 0}},
                    {{fx + fw/2, fy + fh/2}, topColor, {0, 0}},
                    {{fx + fw, fy}, topColor, {0, 0}},
                };
                SDL_RenderGeometry(renderer, nullptr, topVerts, 3, nullptr, 0);
            }

            if (values[1] != values[0] && values[1] != values[2]) {
                SDL_Color leftColor = {leftC.r, leftC.g, leftC.b, leftC.a};
                SDL_Vertex leftVerts[] = {
                    {{fx, fy}, leftColor, {0, 0}},
                    {{fx, fy + fh}, leftColor, {0, 0}},
                    {{fx + fw/2, fy + fh/2}, leftColor, {0, 0}},
                };
                SDL_RenderGeometry(renderer, nullptr, leftVerts, 3, nullptr, 0);
            }
        }
    }
}

void MapWidget::addLocation(const std::string& id, int x, int y, int size, int borderThickness, int state)
{
    auto it = _locations.find(id);
    if (it != _locations.end()) {
        it->second.pos.push_back( {x, y, size, borderThickness, state} );
    } else {
        _locations[id] = { { {x, y, size, borderThickness, state} } };
    }
}

void MapWidget::setLocationState(const std::string& id, int state, size_t n)
{
    auto it = _locations.find(id);
    if (it != _locations.end() && it->second.pos.size() >= n) {
        it->second.pos[n].state = state;
    }
}


} // namespace
