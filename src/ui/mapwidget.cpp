#include "mapwidget.h"
#include "../core/util.h" // countOf
#include "../uilib/drawhelper.h"
#include <algorithm> // std::max, std::min


namespace Ui {

static constexpr float ZOOM_IN_FACTOR = 1.25f;
static constexpr float ZOOM_OUT_FACTOR = 0.8f;

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
        dstx = left + (_size.width - dstw + 1) / 2;
        dsty = top  + (_size.height - dsth + 1) / 2;
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

        float effectiveScale;
        int srcVisW, srcVisH, dstx, dsty, dstw, dsth;
        calculateZoomedView(0, 0, effectiveScale, srcVisW, srcVisH, dstx, dsty, dstw, dsth);

        // Handle dragging for pan (only when zoomed in)
        if ((buttons & SDL_BUTTON_LMASK) && _zoom > 1.001f) {
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
                // Convert screen delta to image delta
                if (effectiveScale > 0) {
                    _panX = _dragStartPanX - deltaX / effectiveScale;
                    _panY = _dragStartPanY - deltaY / effectiveScale;
                }
            }
            return; // Don't process hover while dragging
        } else {
            _dragging = false;
        }

        int absX = _absX + x;
        int absY = _absY + y;

        // Calculate image screen position (same as in render)
        int imgScreenX = dstx - (int)(_panX * effectiveScale);
        int imgScreenY = dsty - (int)(_panY * effectiveScale);
        float baseScale = effectiveScale / _zoom;

        for (auto locIt = _locations.rbegin(); locIt != _locations.rend(); locIt++) {
            const auto& loc = locIt->second;
            for (const auto& pos: loc.pos) {
                if (pos.state == -1) continue; // hidden
                if (pos.state == 0 && _hideClearedLocations) continue;
                if (pos.state == 2 && _hideUnreachableLocations) continue;

                int innerX, innerY, innerW, innerH, borderSize;
                calculateLocationScreenRect(pos, imgScreenX, imgScreenY, effectiveScale, baseScale,
                                            innerX, innerY, innerW, innerH, borderSize);
                int outerW = innerW + 2 * borderSize;
                int outerH = innerH + 2 * borderSize;

                if (x >= innerX - borderSize && x < innerX - borderSize + outerW &&
                    y >= innerY - borderSize && y < innerY - borderSize + outerH)
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

    this->onClick += { this, [this](void*, int, int, int button) {
        // Middle click resets zoom and pan
        if (button == MouseButton::BUTTON_MIDDLE) {
            _zoom = 1.0f;
            _panX = 0.0f;
            _panY = 0.0f;
        }
    }};

    this->onScroll += { this, [this](void*, int, int scrollY, unsigned) {
        // scrollY > 0 = scroll up = zoom in
        // scrollY < 0 = scroll down = zoom out
        float zoomFactor = (scrollY > 0) ? ZOOM_IN_FACTOR : ZOOM_OUT_FACTOR;
        float newZoom = _zoom * zoomFactor;

        // Clamp to minimum 1.0 (no maximum)
        newZoom = std::max(1.0f, newZoom);

        if (newZoom != _zoom) {
            float effectiveScale;
            int srcVisW, srcVisH, dstx, dsty, dstw, dsth;
            calculateZoomedView(0, 0, effectiveScale, srcVisW, srcVisH, dstx, dsty, dstw, dsth);

            if (effectiveScale > 0) {
                // Calculate mouse position in image coordinates (before zoom change)
                float mouseImgX = _panX + (_lastMouseX - dstx) / effectiveScale;
                float mouseImgY = _panY + (_lastMouseY - dsty) / effectiveScale;

                // Apply new zoom
                _zoom = newZoom;

                // Recalculate effective scale with new zoom
                calculateZoomedView(0, 0, effectiveScale, srcVisW, srcVisH, dstx, dsty, dstw, dsth);

                // Adjust pan so that the point under the mouse stays fixed
                _panX = mouseImgX - (_lastMouseX - dstx) / effectiveScale;
                _panY = mouseImgY - (_lastMouseY - dsty) / effectiveScale;
            } else {
                _zoom = newZoom;
            }
        } else if (scrollY < 0 && _zoom <= 1.001f) {
            // At minimum zoom, scrolling down pans back to center
            // Use percentage of widget size for consistent feel across different image sizes
            constexpr float PAN_STEP_PERCENT = 0.3f; // 30% of widget size per scroll
            float screenStep = std::min(_size.width, _size.height) * PAN_STEP_PERCENT;

            // Convert screen pixels to image pixels
            float effectiveScale;
            int srcVisW, srcVisH, dstx, dsty, dstw, dsth;
            calculateZoomedView(0, 0, effectiveScale, srcVisW, srcVisH, dstx, dsty, dstw, dsth);
            float imageStep = (effectiveScale > 0) ? screenStep / effectiveScale : screenStep;

            float dist = std::sqrt(_panX * _panX + _panY * _panY);
            if (dist <= imageStep) {
                // Close enough, snap to center
                _panX = 0.0f;
                _panY = 0.0f;
            } else {
                // Move imageStep pixels towards center
                float scale = (dist - imageStep) / dist;
                _panX *= scale;
                _panY *= scale;
            }
        }
    }};
}

void MapWidget::calculateZoomedView(int offX, int offY, float& effectiveScale, int& srcVisW, int& srcVisH,
                                     int& dstx, int& dsty, int& dstw, int& dsth)
{
    // Get base sizes (at zoom 1.0, with letterboxing)
    int srcw, srch, baseDstX, baseDstY, baseDstW, baseDstH;
    calculateSizes(offX, offY, srcw, srch, baseDstX, baseDstY, baseDstW, baseDstH);

    if (srcw == 0 || srch == 0 || baseDstW == 0 || baseDstH == 0) {
        effectiveScale = 1.0f;
        srcVisW = srcw;
        srcVisH = srch;
        dstx = baseDstX;
        dsty = baseDstY;
        dstw = baseDstW;
        dsth = baseDstH;
        return;
    }

    // Base scale factor (how the image is scaled at zoom 1.0)
    float baseScale = (float)baseDstW / srcw;

    // Effective scale with zoom
    effectiveScale = baseScale * _zoom;

    // Calculate how much of the widget we can fill
    // Virtual image size = srcw * effectiveScale, srch * effectiveScale
    float virtualW = srcw * effectiveScale;
    float virtualH = srch * effectiveScale;

    // Destination size: fill as much of the widget as possible
    dstw = std::min(_size.width, (int)(virtualW + 0.5f));
    dsth = std::min(_size.height, (int)(virtualH + 0.5f));

    // Center in the widget
    dstx = offX + (_size.width - dstw) / 2;
    dsty = offY + (_size.height - dsth) / 2;

    // Visible portion of the image in image coordinates
    srcVisW = (int)(dstw / effectiveScale + 0.5f);
    srcVisH = (int)(dsth / effectiveScale + 0.5f);

    // Clamp to image bounds
    if (srcVisW > srcw) srcVisW = srcw;
    if (srcVisH > srch) srcVisH = srch;
}

void MapWidget::render(Renderer renderer, int offX, int offY)
{
    _absX = offX + _pos.left; // add this to relative mouse coordinates to get absolute
    _absY = offY + _pos.top; // FIXME: we should provide absolute AND relative mouse position through the Event stack

    // Render background color if set
    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        SDL_Rect r = { offX + _pos.left, offY + _pos.top, _size.width, _size.height };
        SDL_RenderFillRect(renderer, &r);
    }

    ensureTexture(renderer);
    auto tex = _enabled ? _tex : _texBw;
    if (!tex) return;

    // Calculate zoomed view parameters
    float effectiveScale;
    int srcVisW, srcVisH, dstx, dsty, dstw, dsth;
    calculateZoomedView(offX + _pos.left, offY + _pos.top, effectiveScale, srcVisW, srcVisH, dstx, dsty, dstw, dsth);

    if (srcVisW == 0 || dstw == 0) {
        return; // nothing to do
    }

    int imgW = _autoSize.width;
    int imgH = _autoSize.height;

    // Calculate where the full zoomed image would be positioned on screen
    // The point (_panX, _panY) in image coords should appear at (dstx, dsty) on screen
    // So image origin (0,0) appears at:
    int imgScreenX = dstx - (int)(_panX * effectiveScale);
    int imgScreenY = dsty - (int)(_panY * effectiveScale);
    int imgScreenW = (int)(imgW * effectiveScale);
    int imgScreenH = (int)(imgH * effectiveScale);

    // Widget bounds (where we're allowed to draw the map)
    int widgetX = offX + _pos.left;
    int widgetY = offY + _pos.top;
    int widgetW = _size.width;
    int widgetH = _size.height;

    // Calculate intersection between image rect and widget rect
    int visLeft = std::max(imgScreenX, widgetX);
    int visTop = std::max(imgScreenY, widgetY);
    int visRight = std::min(imgScreenX + imgScreenW, widgetX + widgetW);
    int visBottom = std::min(imgScreenY + imgScreenH, widgetY + widgetH);

    if (visLeft < visRight && visTop < visBottom) {
        // Convert visible screen rect back to source image coords
        int srcX = (int)((visLeft - imgScreenX) / effectiveScale);
        int srcY = (int)((visTop - imgScreenY) / effectiveScale);
        int srcW = (int)((visRight - visLeft) / effectiveScale + 0.5f);
        int srcH = (int)((visBottom - visTop) / effectiveScale + 0.5f);

        // Clamp source rect to image bounds
        if (srcX + srcW > imgW) srcW = imgW - srcX;
        if (srcY + srcH > imgH) srcH = imgH - srcY;

        if (srcW > 0 && srcH > 0) {
            SDL_Rect srcRect = { srcX, srcY, srcW, srcH };
            SDL_Rect destRect = { visLeft, visTop, visRight - visLeft, visBottom - visTop };
            SDL_RenderCopy(renderer, tex, &srcRect, &destRect);
        }
    }

    // Clipping rect is the widget area
    SDL_Rect clipRect = { widgetX, widgetY, widgetW, widgetH };
    SDL_RenderSetClipRect(renderer, &clipRect);

    // Base scale for fixed-size location markers (independent of zoom)
    float baseScale = effectiveScale / _zoom;

    // Render locations with zoom/pan adjustment
    for (const auto pass: {0, 1}) {
        for (const auto& pair : _locations) {
            const auto& loc = pair.second;
            for (const auto& pos : loc.pos) {
                int state = (int)pos.state;
                if (state == -1) continue; // hidden
                if (state == 0 && _hideClearedLocations) continue;
                if (state == 2 && _hideUnreachableLocations) continue;

                int innerX, innerY, innerW, innerH, borderSize;
                calculateLocationScreenRect(pos, imgScreenX, imgScreenY, effectiveScale, baseScale,
                                            innerX, innerY, innerW, innerH, borderSize);

                // Skip locations that are outside the widget area
                int outerW = innerW + 2 * borderSize;
                int outerH = innerH + 2 * borderSize;
                if (innerX + outerW < widgetX || innerX > widgetX + widgetW ||
                    innerY + outerH < widgetY || innerY > widgetY + widgetH) {
                    continue;
                }

                const Highlight highlight = pos.highlight;

                if (pass == 0) {
                    // glow
                    if (highlight == Highlight::NONE)
                        continue;
                    const Color c = HighlightColors[highlight];
                    if (pos.shape == Shape::DIAMOND)
                        drawDiamondGlow(renderer, {innerX, innerY}, {innerW, innerH}, c);
                    else if (pos.shape == Shape::TRAPEZOID)
                        drawTrapezoidGlow(renderer, {innerX, innerY}, {innerW, innerH}, c);
                    else
                        drawRectGlow(renderer, {innerX, innerY}, {innerW, innerH}, c);
                } else if (!SplitRects || state < 0 || state >= countOf(triangleValues)) {
                    // uniform shape
                    const Color& c = (state < 0 || state >= countOf(StateColors)) ?
                            StateColors[countOf(StateColors) - 1] : StateColors[state];
                    if (pos.shape == Shape::DIAMOND)
                        drawDiamond(renderer, {innerX, innerY}, {innerW, innerH}, borderSize,
                                c, c, c, c);
                    else if (pos.shape == Shape::TRAPEZOID)
                        drawTrapezoid(renderer, {innerX, innerY}, {innerW, innerH}, borderSize,
                                c, c, c, c);
                    else
                        drawRect(renderer, {innerX, innerY}, {innerW, innerH}, borderSize,
                                c, c, c, c);
                } else {
                    // split shape
                    const int* values = triangleValues[state];
                    const Color& topC = StateColors[values[0]];
                    const Color& leftC = StateColors[values[1]];
                    const Color& botC = StateColors[values[2]];
                    const Color& rightC = StateColors[values[3]];

                    if (pos.shape == Shape::DIAMOND)
                        drawDiamond(renderer, {innerX, innerY}, {innerW, innerH}, borderSize,
                                topC, leftC, botC, rightC);
                    else if (pos.shape == Shape::TRAPEZOID)
                        drawTrapezoid(renderer, {innerX, innerY}, {innerW, innerH}, borderSize,
                                topC, leftC, botC, rightC);
                    else
                        drawRect(renderer, {innerX, innerY}, {innerW, innerH}, borderSize,
                                topC, leftC, botC, rightC);
                }
            }
        }
    }

    // Restore clipping
    SDL_RenderSetClipRect(renderer, nullptr);
}

void MapWidget::calculateLocationScreenRect(const Point& pos, int imgScreenX, int imgScreenY,
                                             float effectiveScale, float baseScale,
                                             int& innerX, int& innerY, int& innerW, int& innerH, int& borderSize)
{
    // Location icon size in screen space (fixed size, independent of zoom)
    innerW = (int)(pos.size * baseScale + 0.5f);
    innerH = (int)(pos.size * baseScale + 0.5f);
    if (innerW < 1) innerW = 1;
    if (innerH < 1) innerH = 1;
    borderSize = (int)(pos.borderThickness * baseScale + 0.5f);
    if (borderSize < 1 && pos.borderThickness > 0) borderSize = 1;

    // Calculate screen position based on image position
    innerX = imgScreenX + (int)(pos.x * effectiveScale) - innerW / 2;
    innerY = imgScreenY + (int)(pos.y * effectiveScale) - innerH / 2;
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
