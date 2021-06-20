#include "window.h"
#include "../core/assets.h"
#include <SDL_syswm.h>

namespace Ui {

//#define DEFAULT_SCALE_QUALITY_HINT "1"

Window::Window(const char *title, SDL_Surface* icon, const Position& pos, const Size& size)
    : Container(0,0,600,400)
{
    _title = title;
    if (size.width>0 && size.height>0) _size = size;
    
    printf("Creating renderer ...");
    _win = SDL_CreateWindow(title,
            pos.left, pos.top,
            _size.width, _size.height, SDL_WINDOW_RESIZABLE);
    if (!_win) {
        printf(" error\n");
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return;
    }
    
    if (icon) setIcon(icon);
    
#ifdef DEFAULT_SCALE_QUALITY_HINT
    SDL_SetHintWithPriority(SDL_HINT_RENDER_SCALE_QUALITY, DEFAULT_SCALE_QUALITY_HINT, SDL_HINT_DEFAULT);
#endif
    
    _ren = SDL_CreateRenderer(_win, -1, 0 /*| SDL_RENDERER_PRESENTVSYNC*/ /*| SDL_RENDERER_ACCELERATED*/);
    if (!_ren) {
        printf(" error\n");
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        return;
    }
    
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(_ren, &info) == 0) {
        printf(" %s\n", info.name);
    } else {
        printf(" unknown\n");
    }
    
    SDL_SetRenderDrawBlendMode(_ren, SDL_BLENDMODE_BLEND);
    
    printf("Loading font ...\n");
    _font = TTF_OpenFont(asset(DEFAULT_FONT_FILE).c_str(), DEFAULT_FONT_SIZE);
    if (!_font) fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
}

Window::~Window()
{
    printf("Destroying window...\n");
    // NOTE: we have to destroy children before destroying the renderer
    clearChildren();
    if (_font) TTF_CloseFont(_font);
    if (_ren) SDL_DestroyRenderer(_ren);
    if (_win) SDL_DestroyWindow(_win);
    _font = nullptr;
    _ren = nullptr;
    _win = nullptr;
}

void Window::clear()
{
    auto& c = _backgroundColor;
    if (SDL_SetRenderDrawColor(_ren, c.r, c.g, c.b, 255) != 0) {
        fprintf(stderr, "Window: error setting draw color: %s\n", SDL_GetError());
    }
    if (SDL_RenderClear(_ren) != 0) {
        fprintf(stderr, "Window: error clearing: %s\n", SDL_GetError());
    }
}
void Window::present()
{
    SDL_RenderPresent(_ren);
}

void Window::render(Renderer renderer, int offX, int offY)
{
    Container::render(renderer, offX, offY);
}

void Window::render()
{
    clear();
    render(_ren, 0, 0);
    present();
}

Window::ID Window::getID()
{
    return SDL_GetWindowID(_win);
}

void Window::setIcon(SDL_Surface *icon)
{
    SDL_SetWindowIcon(_win,icon);
}

void Window::setTitle(const std::string& title)
{
    SDL_SetWindowTitle(_win,title.c_str());
}

void Window::resize(Size size)
{
    SDL_SetWindowSize(_win, size.width, size.height);
    setSize(size);
}

void Window::Raise()
{
    SDL_RaiseWindow(_win);
}

static Position tmp; // FIXME: this is not a good solution
const Position& Window::getPosition() const
{
    SDL_GetWindowPosition(_win, &tmp.left, &tmp.top);
    return tmp;
}
Position Window::getPlacementPosition() const
{
    // this returns window coordinates to use in create_window to restore the window
    // NOTE: window position differs between creation and after on X11 by
    //       ~title bar + border. This works around it.
    
    Position pos;
    int offX=0, offY=0;
    SDL_GetWindowPosition(_win, &pos.left, &pos.top);
    
    SDL_SysWMinfo wminfo = {0};
    SDL_VERSION(&wminfo.version);
    if (SDL_GetWindowWMInfo(_win, &wminfo) && wminfo.subsystem == SDL_SYSWM_X11) {
        SDL_GetWindowBordersSize(_win, &offY, &offX, nullptr, nullptr);
    }
    return { pos.left - offX, pos.top - offY };
}
void Window::setPosition(const Position& pos)
{
    // _pos has to stay {0,0} for the render() to work
    SDL_SetWindowPosition(_win, pos.left, pos.top);
}

void Window::setMinSize(Size size)
{
    Container::setMinSize(size);
    size = size && Size{1680,720};
    SDL_SetWindowMinimumSize(_win, size.width, size.height);
}

int Window::getDisplay() const
{
    return SDL_GetWindowDisplayIndex(_win);
}

std::string Window::getDisplayName() const
{
    const char *name = SDL_GetDisplayName(getDisplay());
    return name ? name : "";
}


Position Window::getPositionOnDisplay() const
{
    SDL_Rect rect = {0};
    SDL_GetDisplayBounds(getDisplay(), &rect);
    auto pos = getPosition();
    return { pos.left - rect.x, pos.top - rect.y };
}

void Window::grabFocus()
{
    SDL_SetWindowInputFocus(_win);
}

} // namsepace
