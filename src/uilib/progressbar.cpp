#include "progressbar.h"
#include "colorhelper.h"


namespace Ui {

constexpr SDL_Color INDICATOR_COLOR = {0, 128, 64, 255};

ProgressBar::ProgressBar(const int x, const int y, const int w, const int h, const int max, const int progress)
    : Widget(x, y, w, h), _max(max), _progress(progress)
{
}

void ProgressBar::render(Renderer renderer, const int offX, const int offY)
{
    if (_size.width < 2 || _size.height < 2)
        return;

    SDL_Rect r = {
        offX+_pos.left,
        offY+_pos.top,
        _size.width,
        _size.height
    };

    constexpr SDL_Color c1 = INDICATOR_COLOR;
    constexpr SDL_Color c2 = setSaturation<BWMode::Luminosity>(c1, 0.5);

    if (_max >= 0) {
        // linear progress
        const int64_t w = r.w;
        // indicator
        r.w = static_cast<int>(_max ? w * _progress / _max : w);
        SDL_SetRenderDrawColor(renderer, c1.r, c1.g, c1.b, c1.a);
        SDL_RenderFillRect(renderer, &r);
        // track
        r.x += r.w;
        r.w = static_cast<int>(w - r.w);
        SDL_SetRenderDrawColor(renderer, c2.r, c2.g, c2.b, 63);
        SDL_RenderFillRect(renderer, &r);
        _firstIndeterminateDraw = 0;
    } else {
        // indeterminate progress
        const tick_t now = getTicks();
        int p;
        if (_firstIndeterminateDraw == 0) {
            _firstIndeterminateDraw = now;
            p = 0;
        } else {
            p = static_cast<int>((now - _firstIndeterminateDraw) / 7) % (_size.width + _size.width / 2);
        }
        const int x0 = r.x;
        // left end of indicator stays at 0 until half and then starts moving
        const int x1 = x0 + std::min(_size.width, std::max(0, p - _size.width / 2));
        // right end of indicator moves linearly
        const int x2 = x0 + std::min(_size.width, p);
        const int x3 = x0 + r.w;

        // indicator
        r.x = x1;
        r.w = x2 - x1;
        SDL_SetRenderDrawColor(renderer, c1.r, c1.g, c1.b, c1.a);
        SDL_RenderFillRect(renderer, &r);
        // track left
        r.x = x0;
        r.w = x1 - x0;
        SDL_SetRenderDrawColor(renderer, c2.r, c2.g, c2.b, 63);
        SDL_RenderFillRect(renderer, &r);
        // track right
        r.x = x2;
        r.w = x3 - x2;
        SDL_SetRenderDrawColor(renderer, c2.r, c2.g, c2.b, 63);
        SDL_RenderFillRect(renderer, &r);
    }
}

void ProgressBar::setMax(const int max)
{
    if (_max == max)
        return;
    _max = max;
}

void ProgressBar::setProgress(const int progress)
{
    if (_progress == progress)
        return;
    _progress = progress;
}

} // namespace
