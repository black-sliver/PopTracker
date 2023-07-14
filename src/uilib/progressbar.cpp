#include "progressbar.h"


namespace Ui {

ProgressBar::ProgressBar(int x, int y, int w, int h, int max, int progress)
    : Widget(x, y, w, h), _max(max), _progress(progress)
{
}

ProgressBar::~ProgressBar()
{
}

void ProgressBar::render(Renderer renderer, int offX, int offY)
{
    if (_size.width < 2 || _size.height < 2) return;
    // border
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_Rect r = { offX+_pos.left,
                   offY+_pos.top,
                   _size.width,
                   _size.height
    };
    SDL_RenderDrawRect(renderer, &r);
    r.x += 1;
    r.y += 1;
    r.w -= 2;
    r.h -= 2;
    if (_max >= 0) {
        int64_t w = r.w;
        //fill progress
        r.w = _max ? (int)(w * _progress / _max) : w;
        SDL_SetRenderDrawColor(renderer, 0, 128, 64, 255);
        SDL_RenderFillRect(renderer, &r);
        //fill missing
        r.x += r.w;
        r.w = (int)(w-r.w);
        SDL_SetRenderDrawColor(renderer, 48, 48, 48, 255);
        SDL_RenderFillRect(renderer, &r);        
    } else {
        // TODO: sliding bar
        SDL_SetRenderDrawColor(renderer, 0, 128, 64, 255);
        SDL_RenderFillRect(renderer, &r);
    }
}

void ProgressBar::setMax(int max)
{
    if (_max == max) return;
    _max = max;
}
void ProgressBar::setProgress(int progress)
{
    if (_progress == progress) return;
    _progress = progress;
}
    
} // namespace
