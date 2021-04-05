#include "mapwidget.h"
#include "../core/util.h" // countOf

namespace Ui {

static const Widget::Color STATE_COLOR[] {
    // done
    { 0x3f, 0x3f, 0x3f },
    // available -> green
    { 0x20, 0xff, 0x20 },
    // unavailable -> darkish red
    { 0xcf, 0x10, 0x10 },
    // mixed available/unavailable -> light orange
    { 0xff, 0x9f, 0x20 },
    // glitches required -> yellow
    { 0xff, 0xff, 0x20 },
    // mixed available + glitches -> greenish yellow
    { 0xaf, 0xff, 0x20 },
    // mixed unavailable + glitches -> dark orange
    { 0xef, 0x55, 0x00 },
    // mixed all three -> light orange
    { 0xff, 0x9f, 0x20 },
    
    // checkable -> blue
    { 0x30, 0x40, 0xff },
    // checkable + available -> teal
    { 0x20, 0xff, 0xff },
    // checkable + unavailable -> purple
    { 0xc0, 0x10, 0xff },
    
    // mixed checkable + available/unavailable -> light orange
    { 0xff, 0x9f, 0x20 },
    // mixed checkable + glitches required -> i don't know :( -> darker teal?
    { 0x20, 0xd0, 0xd0 },
    // mixed checkable + available + glitches -> light orange
    { 0xff, 0x9f, 0x20 },
    // mixed checkable + unavailable + glitches -> i don't know :( -> darker teal?
    { 0x20, 0xd0, 0xd0 },
    
    // mixed all 4 -> greenish yellow
    { 0xff, 0xff, 0x00, 0xff },
    
    
    
    // mixed checkable + available -> green
    // mixed 
    
    // error
    { 0x00, 0x00, 0x00, 0xff },
};

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
        int locsize = _locationSize + 2 * _locationBorder; // or without border?
        bool match = false;
        for (auto locIt = _locations.rbegin(); locIt!=_locations.rend(); locIt++) {
            const auto& loc = locIt->second;
            for (const auto& pos: loc.pos) {
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
    // location icon size in screen space
    int locScreenInnerW  = (_locationSize*dstw+srcw/2)/srcw;
    int locScreenInnerH  = (_locationSize*dsth+srch/2)/srch;
    if (locScreenInnerW<1) locScreenInnerW=1;
    if (locScreenInnerH<1) locScreenInnerH=1;
    int borderScreenSize = (_locationBorder*dstw+srcw/2)/srcw;
    if (borderScreenSize<1 && _locationBorder>0) borderScreenSize=1;
    int locScreenOuterW  = locScreenInnerW+2*borderScreenSize;
    int locScreenOuterH  = locScreenInnerH+2*borderScreenSize;
    
    for (const auto& pair : _locations) {
        const auto& loc = pair.second;
        for (const auto& pos : loc.pos) {
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

            int state = (int)loc.state;
            const Widget::Color& c = (state<0 || state>=countOf(STATE_COLOR)) ?
                    STATE_COLOR[countOf(STATE_COLOR)-1] : STATE_COLOR[state];

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderFillRect(renderer, &outer);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_RenderFillRect(renderer, &inner);
        }
    }
}

void MapWidget::addLocation(const std::string& id, int x, int y, int state)
{
    auto it = _locations.find(id);
    if (it != _locations.end()) {
        it->second.pos.push_back( {x,y} );
    } else {
        _locations[id] = { { {x,y} },state};
    }
}
void MapWidget::setLocationState(const std::string& id, int state)
{
    auto it = _locations.find(id);
    if (it != _locations.end()) {
        it->second.state = state;
    }
}


} // namespace
