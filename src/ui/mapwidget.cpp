#include "mapwidget.h"
#include "../core/util.h" // countOf
#include "../uilib/drawhelper.h"
#include <algorithm> // std::max, std::min


namespace Ui {

static constexpr float ZOOM_IN_FACTOR = 1.25f;
static constexpr float ZOOM_OUT_FACTOR = 0.8f;
static constexpr float MAX_ZOOM = 1000.0f;

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

void MapWidget::connectSignals()
{
    this->onMouseMove += { this, [this](void*, const int x, const int y, const unsigned buttons) {
        // Track mouse position for scroll zooming
        _lastMouseX = x + _pos.left; // relative to parent to match dstRect
        _lastMouseY = y + _pos.top;

        if (_size.width < 1 || _size.height < 1)
            return;

        // Calculate image screen position (same as in render)
        float baseScale;
        SDL_Rect srcRect;
        SDL_FRect dstRect;
        calculateSrcAndDst(0, 0, false, baseScale, srcRect, dstRect);
        const float scale = dstRect.w / static_cast<float>(srcRect.w);

        // Handle dragging for pan (only when zoomed in or not at 0,0)
        if (buttons & SDL_BUTTON_LMASK) {
            if (!_dragging && (_zoom > 1.0f || _panX != 0.0f || _panY != 0.0f)) {
                // Start drag
                _dragging = true;
                _dragStartX = x;
                _dragStartY = y;
                _dragStartPanX = _panX;
                _dragStartPanY = _panY;
            } else if (_dragging) {
                // During drag: calculate delta and update pan
                const int deltaX = _dragStartX - x;
                const int deltaY = _dragStartY - y;
                if (scale > 0) {
                    // Convert screen delta to image delta
                    float newPanX = _dragStartPanX - static_cast<float>(deltaX) / scale;
                    float newPanY = _dragStartPanY - static_cast<float>(deltaY) / scale;
                    // if not zoomed, snap to 0 by not allowing to flip sign from drag start
                    if (_zoom <= 1.0f && ((_dragStartPanX >= 0.0f && newPanX < 0.0f) || (_dragStartPanX <= 0.0f && newPanX > 0.0f))) {
                        newPanX = 0.0f;
                    }
                    if (_zoom <= 1.0f && ((_dragStartPanY >= 0.0f && newPanY < 0.0f) || (_dragStartPanY <= 0.0f && newPanY > 0.0f))) {
                        newPanY = 0.0f;
                    }
                    _panX = newPanX;
                    _panY = newPanY;
                }
            }
            return; // Don't process hover while dragging
        }

        _dragging = false;

        const int absX = _absX + x;
        const int absY = _absY + y;

        const int x1 = x + _pos.left; // relative to parent to match dstRect
        const int y1 = y + _pos.top;

        for (auto locIt = _locations.rbegin(); locIt != _locations.rend(); ++locIt) {
            const auto& loc = locIt->second;
            for (const auto& pos: loc.pos) {
                if (pos.state == -1) continue; // hidden
                if (pos.state == 0 && _hideClearedLocations) continue;
                if (pos.state == 2 && _hideUnreachableLocations) continue;

                int innerX, innerY, innerW, innerH, borderSize;
                calculateLocationScreenRect(pos, srcRect, dstRect, baseScale,
                    innerX, innerY, innerW, innerH, borderSize);
                const int outerW = innerW + 2 * borderSize;
                const int outerH = innerH + 2 * borderSize;

                if (x1 >= innerX - borderSize && x1 < innerX - borderSize + outerW &&
                    y1 >= innerY - borderSize && y1 < innerY - borderSize + outerH)
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

    this->onScroll += { this, [this](void*, int, const int scrollY, unsigned) {
        // scrollY > 0 = scroll up = zoom in
        // scrollY < 0 = scroll down = zoom out

        if (_size.width < 1 || _size.height < 1)
            return;

        const float zoomFactor = (scrollY > 0) ? ZOOM_IN_FACTOR : ZOOM_OUT_FACTOR;
        float newZoom = _zoom * zoomFactor;

        // Clamp to 1.0 - 1000.0
        if (newZoom < 1.01f) {
            newZoom = 1.0f;
        } else if (newZoom > MAX_ZOOM) {
            newZoom = MAX_ZOOM;
        }

        // Calculate image screen position (same as in render)
        float baseScale;
        SDL_Rect oldSrcRect;
        SDL_FRect oldDstRect;
        calculateSrcAndDst(0, 0, false, baseScale, oldSrcRect, oldDstRect);
        if (zoomFactor > 1.0f && (oldSrcRect.w <= 1 || oldSrcRect.h <= 1))
            return; // don't zoom in further // TODO: estimate this without calculateSrcAndDst
        const float oldScale = oldDstRect.w / static_cast<float>(oldSrcRect.w);

        if (newZoom != _zoom) {
            if (oldScale == 0.0f) {
                // should never happen, but pan update would be a division by 0
                _zoom = newZoom;
            } else {
                // Calculate mouse position in image coordinates (before zoom change)
                const float oldMouseImgX = (_lastMouseX - oldDstRect.x) / oldScale + oldSrcRect.x;
                const float oldMouseImgY = (_lastMouseY - oldDstRect.y) / oldScale + oldSrcRect.y;

                // Apply new zoom
                _zoom = newZoom;

                // Recalculate effective scale with new zoom
                SDL_Rect newSrcRect;
                SDL_FRect newDstRect;
                calculateSrcAndDst(0, 0, false, baseScale, newSrcRect, newDstRect);
                const float newScale = newDstRect.w / static_cast<float>(newSrcRect.w);

                // Adjust pan so that the point under the mouse stays fixed
                const float newMouseImgX = (_lastMouseX - newDstRect.x) / newScale + newSrcRect.x;
                const float newMouseImgY = (_lastMouseY - newDstRect.y) / newScale + newSrcRect.y;
                _panX += newMouseImgX - oldMouseImgX;
                _panY += newMouseImgY - oldMouseImgY;
            }
        } else if (scrollY < 0 && _zoom == 1.0f) {
            // At minimum zoom, scrolling down pans back to center
            // Use percentage of widget size for consistent feel across different image sizes
            constexpr float PAN_STEP_PERCENT = 0.3f; // 30% of widget size per scroll
            const float screenStep = static_cast<float>(std::min(_size.width, _size.height)) * PAN_STEP_PERCENT;

            // Convert screen pixels to image pixels
            const float imageStep = (baseScale > 0) ? screenStep * baseScale : screenStep;

            const float dist = std::sqrt(_panX * _panX + _panY * _panY);
            if (dist <= imageStep) {
                // Close enough, snap to center
                _panX = 0.0f;
                _panY = 0.0f;
            } else {
                // Move imageStep pixels towards center
                const float panScale = (dist - imageStep) / dist;
                _panX *= panScale;
                _panY *= panScale;
            }
        }
    }};
}

void MapWidget::render(Renderer renderer, const int offX, const int offY)
{
    _absX = offX + _pos.left; // add this to relative mouse coordinates to get absolute
    _absY = offY + _pos.top; // FIXME: we should provide absolute AND relative mouse position through the Event stack

    if (_size.width < 1 || _size.height < 1)
        return;

    // Widget bounds (where we're allowed to draw the map)
    const int widgetX = offX + _pos.left;
    const int widgetY = offY + _pos.top;
    const int widgetW = _size.width;
    const int widgetH = _size.height;

    // Render background color if set
    if (_backgroundColor.a > 0) {
        const auto& c = _backgroundColor;
        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
        const SDL_Rect r = { widgetX, widgetY, widgetW, widgetH };
        SDL_RenderFillRect(renderer, &r);
    }

    ensureTexture(renderer);
    const auto tex = _enabled ? _tex : _texBw;
    if (!tex)
        return;

    // Clipping rect is the widget area
    const SDL_Rect clipRect = { widgetX, widgetY, widgetW, widgetH };
    SDL_RenderSetClipRect(renderer, &clipRect);

    SDL_Rect srcRect;
    SDL_FRect dstRect;
    float baseScale;
    calculateSrcAndDst(offX, offY, true, baseScale, srcRect, dstRect);
    SDL_RenderCopyF(renderer, tex, &srcRect, &dstRect);

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
                calculateLocationScreenRect(pos, srcRect, dstRect, baseScale, innerX, innerY, innerW, innerH, borderSize);

                // Skip locations that are outside the widget area
                const int outerW = innerW + 2 * borderSize;
                const int outerH = innerH + 2 * borderSize;
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

void MapWidget::calculateSrcAndDst(const int offX, const int offY, const bool clip, float& baseScale, SDL_Rect& srcRect,
    SDL_FRect& dstRect) const
{
    const auto imgW = static_cast<float>(_autoSize.width);
    const auto imgH = static_cast<float>(_autoSize.height);
    const auto viewportW = static_cast<float>(_size.width);
    const auto viewportH = static_cast<float>(_size.height);
    // the image coordinates that should be in the center of the widget
    const float centerX = imgW / 2 - _panX;
    const float centerY = imgH / 2 - _panY;
    // If imgW/imgH > widgetW/widgetH, letterbox will be top/bottom and the reference size is imgW,
    // otherwise letterbox will be left/right and the reference size is imgH.
    // Calculate "unzoomed" src view rect and base scale based on viewport aspect ratio.
    // Base scale is chosen so that at 1x zoom, the whole image fits inside the widget.
    // Note that the src view rect can be out of bounds and will have to be clipped at the end.
    baseScale = (imgW / viewportW > imgH / viewportH) ? imgW / viewportW : imgH / viewportH;
    float srcW = baseScale * viewportW / _zoom;
    float srcH = baseScale * viewportH / _zoom;
    float srcX = centerX - srcW / 2;
    float srcY = centerY - srcH / 2;
    auto dstX = static_cast<float>(_pos.left + offX);
    auto dstY = static_cast<float>(_pos.top + offY);
    float dstW = viewportW;
    float dstH = viewportH;

    // srcRect needs to be integer, so floor/ceil src coordinates and update dst accordingly
    const float fracX = srcX - std::floor(srcX);
    srcX = std::floor(srcX);
    const float fracY = srcY - std::floor(srcY);
    srcY = std::floor(srcY);
    const float fracW = std::ceil(srcW + fracX) - srcW;
    srcW = std::ceil(srcW + fracX);
    const float fracH = std::ceil(srcH + fracY) - srcH;
    srcH = std::ceil(srcH + fracY);
    dstX -= fracX * _zoom / baseScale;
    dstY -= fracY * _zoom / baseScale;
    dstW += fracW * _zoom / baseScale;
    dstH += fracH * _zoom / baseScale;

    if (clip) {
        // render needs src to be in range 0...size, so we need to clip src and update dst if src is out of bounds
        if (srcX < 0) {
            dstX -= srcX * _zoom / baseScale;
            dstW += srcX * _zoom / baseScale;
            srcW += srcX;
            srcX = 0;
        }
        if (srcY < 0) {
            dstY -= srcY * _zoom / baseScale;
            dstH += srcY * _zoom / baseScale;
            srcH += srcY;
            srcY = 0;
        }
        if (srcX + srcW > imgW) {
            srcW = imgW - srcX;
            dstW = srcW * _zoom / baseScale;
        }
        if (srcY + srcH > imgH) {
            srcH = imgH - srcY;
            dstH = srcH * _zoom / baseScale;
        }
    }

    srcRect = {
        static_cast<int>(std::lround(srcX)),
        static_cast<int>(std::lround(srcY)),
        std::max(1, static_cast<int>(std::lround(srcW))),
        std::max(1, static_cast<int>(std::lround(srcH))),
    };
    dstRect = {
        dstX,
        dstY,
        std::max(1.0f, dstW),
        std::max(1.0f, dstH),
    };
}

void MapWidget::calculateLocationScreenRect(const Point& pos, const SDL_Rect& srcRect, const SDL_FRect& dstRect,
    const float baseScale, int& innerX, int& innerY, int& innerW, int& innerH, int& borderSize)
{
    // Location icon size in screen space (fixed size, independent of zoom)
    innerW = static_cast<int>(lround(static_cast<float>(pos.size) / baseScale));
    innerH = static_cast<int>(lround(static_cast<float>(pos.size) / baseScale));
    if (innerW < 1) innerW = 1;
    if (innerH < 1) innerH = 1;
    borderSize = static_cast<int>(lround(static_cast<float>(pos.borderThickness) / baseScale));
    if (borderSize < 1 && pos.borderThickness > 0)
        borderSize = 1;

    // Calculate screen position based on image position
    const float scale = static_cast<float>(dstRect.w) / static_cast<float>(srcRect.w);
    innerX = static_cast<int>(static_cast<float>(pos.x - srcRect.x) * scale) + dstRect.x - innerW / 2;
    innerY = static_cast<int>(static_cast<float>(pos.y - srcRect.y) * scale) + dstRect.y - innerH / 2;
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

std::tuple<float, float> MapWidget::getPanCenter() const
{
    return {
        static_cast<float>(_autoSize.width) / 2 - _panX,
        static_cast<float>(_autoSize.height) / 2 - _panY,
    };
}

void MapWidget::setPanCenter(const float x, const float y)
{
    _panX = static_cast<float>(_autoSize.width) / 2 - x;
    _panY = static_cast<float>(_autoSize.height) / 2 - y;
}

} // namespace
