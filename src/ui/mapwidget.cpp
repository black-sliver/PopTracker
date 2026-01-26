#include "mapwidget.h"
#include "../core/util.h" // countOf
#include "../uilib/drawhelper.h"
#include "../uilib/colorhelper.h" // makeGreyscale
#include <algorithm> // std::max, std::min


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

#define DEFAULT_HIGHLIGHT_COLORS { \
    { Highlight::AVOID, {0xff, 0x00, 0x00, 0xcc}}, \
    { Highlight::NONE, {0x00, 0x00, 0x00, 0x00}}, \
    { Highlight::NO_PRIORITY, {0x6a, 0x7f, 0xf8, 0xcc}}, /* slateblue-ish */ \
    { Highlight::UNSPECIFIED, {0xff, 0xff, 0xff, 0xbb}}, \
    { Highlight::PRIORITY, {0xff, 0xd7, 0x00, 0xcc}}, /* plum is hard to see, so we make it "gold" */ \
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
std::map<Highlight, Widget::Color> MapWidget::HighlightColors = DEFAULT_HIGHLIGHT_COLORS;

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
    this->onMouseMove += { this, [this](void*, int x, int y, unsigned buttons) {
        // Track mouse position for scroll zooming
        _lastMouseX = x;
        _lastMouseY = y;

        int srcw, srch, dstx, dsty, dstw, dsth;
        calculateSizes(0, 0, srcw, srch, dstx, dsty, dstw, dsth);

        // Handle dragging for pan
        if (buttons & SDL_BUTTON_LMASK) {
            if (!_dragging) {
                // Start drag
                _dragging = true;
                _dragStartX = x;
                _dragStartY = y;
                _dragStartPanX = _panX;
                _dragStartPanY = _panY;
            } else {
                // During drag: calculate delta and update pan
                int deltaX = x - _dragStartX;
                int deltaY = y - _dragStartY;
                // Convert screen delta to image delta (inverse of zoom)
                if (dstw > 0 && dsth > 0) {
                    _panX = _dragStartPanX - (deltaX * srcw) / (dstw * _zoom);
                    _panY = _dragStartPanY - (deltaY * srch) / (dsth * _zoom);
                    clampPan();
                }
            }
            return; // Don't process hover while dragging
        } else {
            _dragging = false;
        }

        int absX = _absX + x;
        int absY = _absY + y;

        // Convert screen coordinates to image coordinates (accounting for zoom and pan)
        int x1 = x - dstx;
        int y1 = y - dsty;
        // Screen to image: imgCoord = panOffset + (screenCoord * imgSize) / (displaySize * zoom)
        if (dstw > 0 && dsth > 0) {
            x1 = (int)(_panX + (x1 * srcw) / (dstw * _zoom));
            y1 = (int)(_panY + (y1 * srch) / (dsth * _zoom));
        }

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
                    if (locIt->first != _locationHover) {
                        _locationHover = locIt->first;
    #if 0
                        printf("MapWidget: hover location %s\n", _locationHover.c_str());
    #endif
                        onLocationHover.emit(this, _locationHover, absX, absY);
                    }
                    return;
                }
            }
        }
        // no match
        _locationHover = "";
    }};

    this->onMouseLeave += { this, [this](void*) {
        _locationHover = "";
        _dragging = false;
    }};

    this->onScroll += { this, [this](void*, int, int scrollY, unsigned) {
        // scrollY > 0 = scroll up = zoom in
        // scrollY < 0 = scroll down = zoom out
        float zoomFactor = (scrollY > 0) ? 1.25f : 0.8f;
        float newZoom = _zoom * zoomFactor;

        // Clamp between 1.0 and 8.0
        newZoom = std::max(1.0f, std::min(8.0f, newZoom));

        if (newZoom != _zoom) {
            int srcw, srch, dstx, dsty, dstw, dsth;
            calculateSizes(0, 0, srcw, srch, dstx, dsty, dstw, dsth);

            if (dstw > 0 && dsth > 0) {
                // Calculate mouse position in image coordinates (before zoom change)
                // Use last known mouse position from onMouseMove
                int mouseLocalX = _lastMouseX - dstx;
                int mouseLocalY = _lastMouseY - dsty;
                float mouseImgX = _panX + (mouseLocalX * srcw) / (dstw * _zoom);
                float mouseImgY = _panY + (mouseLocalY * srch) / (dsth * _zoom);

                // Apply new zoom
                _zoom = newZoom;

                // Adjust pan so that the point under the mouse stays fixed
                _panX = mouseImgX - (mouseLocalX * srcw) / (dstw * _zoom);
                _panY = mouseImgY - (mouseLocalY * srch) / (dsth * _zoom);
                clampPan();
            } else {
                _zoom = newZoom;
                clampPan();
            }
        }
    }};
}

void MapWidget::clampPan()
{
    // Calculate pan limits based on zoom level
    // At zoom 1.0, pan must be 0 (show entire image)
    // At higher zoom, we can pan but not beyond image edges
    float maxPanX = _autoSize.width * (1.0f - 1.0f / _zoom);
    float maxPanY = _autoSize.height * (1.0f - 1.0f / _zoom);
    _panX = std::max(0.0f, std::min(maxPanX, _panX));
    _panY = std::max(0.0f, std::min(maxPanY, _panY));
}

void MapWidget::render(Renderer renderer, int offX, int offY)
{
    _absX = offX+_pos.left; // add this to relative mouse coordinates to get absolute
    _absY = offY+_pos.top; // FIXME: we should provide absolute AND relative mouse position through the Event stack

    // Render background color if set
    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect r = { offX+_pos.left, offY+_pos.top, _size.width, _size.height };
        SDL_RenderFillRect(renderer, &r);
    }

    // Create texture from surface if needed (same as Image::render)
    if (!_tex && _surf) {
        if (_quality >= 0) {
            char q[] = { (char)('0'+_quality), 0 };
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, q);
        }
        _tex = SDL_CreateTextureFromSurface(renderer, _surf);
        _surf = makeGreyscale(_surf, _darkenGreyscale);
        _texBw = SDL_CreateTextureFromSurface(renderer, _surf);
        SDL_FreeSurface(_surf);
        if (_quality >= 0) {
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "");
        }
        _surf = nullptr;
    }
    auto tex = _enabled ? _tex : _texBw;
    if (!tex) return;

    int srcw, srch, dstx, dsty, dstw, dsth;
    calculateSizes(offX+_pos.left, offY+_pos.top, srcw, srch, dstx, dsty, dstw, dsth);
    if (srcw == 0 || dstw == 0) {
        return; // nothing to do
    }

    // Calculate source rect based on zoom and pan
    // The visible portion of the image in image coordinates
    int visibleW = (int)(srcw / _zoom);
    int visibleH = (int)(srch / _zoom);
    SDL_Rect srcRect = {
        (int)_panX,
        (int)_panY,
        visibleW,
        visibleH
    };
    SDL_Rect destRect = { dstx, dsty, dstw, dsth };
    SDL_RenderCopy(renderer, tex, &srcRect, &destRect);

    // Render locations with zoom/pan adjustment
    for (const auto pass: {0, 1}) {
        for (const auto& pair : _locations) {
            const auto& loc = pair.second;
            for (const auto& pos : loc.pos) {
                int state = (int)pos.state;
                if (state == -1) continue; // hidden
                if (state == 0 && _hideClearedLocations) continue;
                if (state == 2 && _hideUnreachableLocations) continue;

                // location icon size in screen space (scaled by zoom)
                int locScreenInnerW  = (int)((pos.size * dstw * _zoom + srcw/2) / srcw);
                int locScreenInnerH  = (int)((pos.size * dsth * _zoom + srch/2) / srch);
                if (locScreenInnerW<1) locScreenInnerW=1;
                if (locScreenInnerH<1) locScreenInnerH=1;
                int borderScreenSize = (int)((pos.borderThickness * dstw * _zoom + srcw/2) / srcw);
                if (borderScreenSize<1 && pos.borderThickness>0) borderScreenSize=1;

                // Calculate screen position: (imgPos - panOffset) * zoom * scaleFactor
                // Screen position = dstx + ((imgX - panX) * dstw * zoom) / srcw
                int innerx = dstx + (int)(((pos.x - _panX) * dstw * _zoom) / srcw) - locScreenInnerW/2;
                int innery = dsty + (int)(((pos.y - _panY) * dsth * _zoom) / srch) - locScreenInnerH/2;

                // Skip locations that are outside the visible area
                int locScreenOuterW = locScreenInnerW + 2*borderScreenSize;
                int locScreenOuterH = locScreenInnerH + 2*borderScreenSize;
                if (innerx + locScreenOuterW < dstx || innerx > dstx + dstw ||
                    innery + locScreenOuterH < dsty || innery > dsty + dsth) {
                    continue;
                }

                const Highlight highlight = pos.highlight;

                if (pass == 0) {
                    // glow
                    if (highlight == Highlight::NONE)
                        continue;
                    const Color c = HighlightColors[highlight];
                    if (pos.shape == Shape::DIAMOND)
                        drawDiamondGlow(renderer, {innerx, innery}, {locScreenInnerW, locScreenInnerH}, c);
                    else if (pos.shape == Shape::TRAPEZOID)
                        drawTrapezoidGlow(renderer, {innerx, innery}, {locScreenInnerW, locScreenInnerH}, c);
                    else
                        drawRectGlow(renderer, {innerx, innery}, {locScreenInnerW, locScreenInnerH}, c);
                } else if (!SplitRects || state<0 || state>=countOf(triangleValues)) {
                    // uniform shape
                    const Color& c = (state<0 || state>=countOf(StateColors)) ?
                            StateColors[countOf(StateColors)-1] : StateColors[state];
                    if (pos.shape == Shape::DIAMOND)
                        drawDiamond(renderer, {innerx, innery}, {locScreenInnerW, locScreenInnerH}, borderScreenSize,
                                c, c, c, c);
                    else if (pos.shape == Shape::TRAPEZOID)
                        drawTrapezoid(renderer, {innerx, innery}, {locScreenInnerW, locScreenInnerH}, borderScreenSize,
                                c, c, c, c);
                    else
                        drawRect(renderer, {innerx, innery}, {locScreenInnerW, locScreenInnerH}, borderScreenSize,
                                c, c, c, c);
                } else {
                    // split shape
                    const int* values = triangleValues[state];
                    const Color& topC = StateColors[values[0]];
                    const Color& leftC = StateColors[values[1]];
                    const Color& botC = StateColors[values[2]];
                    const Color& rightC = StateColors[values[3]];

                    if (pos.shape == Shape::DIAMOND)
                        drawDiamond(renderer, {innerx, innery}, {locScreenInnerW, locScreenInnerH}, borderScreenSize,
                                topC, leftC, botC, rightC);
                    else if (pos.shape == Shape::TRAPEZOID)
                        drawTrapezoid(renderer, {innerx, innery}, {locScreenInnerW, locScreenInnerH}, borderScreenSize,
                                topC, leftC, botC, rightC);
                    else
                        drawRect(renderer, {innerx, innery}, {locScreenInnerW, locScreenInnerH}, borderScreenSize,
                                topC, leftC, botC, rightC);
                }
            }
        }
    }
}

void MapWidget::addLocation(const std::string& id, Point&& point)
{
    auto it = _locations.find(id);
    if (it != _locations.end()) {
        it->second.pos.emplace_back(point);
    } else {
        _locations[id] = {{point}};
    }
}

void MapWidget::setLocationState(const std::string& id, int state, size_t n)
{
    auto it = _locations.find(id);
    if (it != _locations.end() && it->second.pos.size() >= n) {
        it->second.pos[n].state = state;
    }
}

void MapWidget::setLocationHighlight(const std::string& id, Highlight highlight, size_t n)
{
    auto it = _locations.find(id);
    if (it != _locations.end() && it->second.pos.size() >= n) {
        it->second.pos[n].highlight = highlight;
    }
}


} // namespace
