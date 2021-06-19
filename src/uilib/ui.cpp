#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <unistd.h>
#include <time.h> 
#include <stdint.h>


#define LIMIT_FPS 120


static uint64_t getMicroTicks()
{
    timespec tp;
     if (clock_gettime(CLOCK_MONOTONIC, &tp) != 0) return 0;
    uint64_t micros = tp.tv_sec; micros *= 1000000;
    micros += tp.tv_nsec/1000;
    return micros;
}


namespace Ui {

Ui::Ui(const char *name)
{
    _name = name;
    SDL_version v;
    SDL_GetVersion(&v);
    printf("Ui: Init SDL %hhu.%hhu.%hhu...\n", v.major, v.minor, v.patch);
    if (SDL_WasInit(SDL_INIT_VIDEO)) fprintf(stderr, "Warning: SDL already initialized!\n");
#if 0
    else SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
#endif
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "Error initializing SDL_TTF: %s\n", TTF_GetError());
    }

#ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR /* available beginning SDL 2.0.8 */
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif

#if 0
    // set OpenGL defaults in case opengl renderer is used
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    //SDL_GL_SetSwapInterval(-1); // adaptive vsync
#endif
}

Ui::~Ui()
{
    printf("Ui: Destroying UI...\n");
    for (auto win: _windows)
        delete win.second;
    printf("Ui: Destroying SDL...\n");
    TTF_Quit();
    SDL_Quit();
}

void Ui::destroyWindow(Window *win)
{
    if (!win) return;
    for (auto it = _windows.begin(); it != _windows.end(); it++) {
        if (it->second == win) {
            _windows.erase(it);
            delete win;
            break;
        }
    }
}

bool Ui::render()
{
    // FPS limiter:
    // stay as long in the event loop as possible. sleep to switch tasks.
    // if not using vsync redraw ASAP for destructive events
    // browser context/emscripten: similar to vsync, but waiting is bad
    
    // FIXME: defining LIMIT_FPS may result in an endless-loop
    
#ifdef LIMIT_FPS
    #define FRAME_TIME (1000/LIMIT_FPS) // TODO: microseconds
#endif
    
    uint32_t t0 = SDL_GetTicks(); // TODO: microseconds
    uint32_t t1 = t0;
    
    do {
#ifndef __EMSCRIPTEN__
        SDL_Delay(1); // let the kernel switch tasks to fill the event queue
#endif
        bool destructiveEvent = false;
        
        // Work around SDL eating mouse input when switching windows
        // read below at SDL_WINDOWEVENT_FOCUS_GAINED
        if (_globalMouseButton) {
            if (!SDL_GetGlobalMouseState(nullptr, nullptr)) {
                // untracked mouse button released ...
                SDL_Window* win = SDL_GetMouseFocus();
                if (win) {
                    // ... inside window -> fire event
                    int x,y;
                    SDL_GetMouseState(&x, &y);
                    SDL_Event ev; memset(&ev, 0, sizeof(ev));
                    ev.type = SDL_MOUSEBUTTONDOWN;
                    ev.button.x = (Sint32)x;
                    ev.button.y = (Sint32)y;
                    ev.button.button = (Uint8) _globalMouseButton;
                    ev.button.windowID = SDL_GetWindowID(win);
                    SDL_PushEvent(&ev);
                    ev.type = SDL_MOUSEBUTTONUP;
                    SDL_PushEvent(&ev);
                }
                _globalMouseButton = 0;
            }
        }
        
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT: {
                    printf("Ui: Quit\n");
                    return false;
                }
                case SDL_MOUSEBUTTONDOWN: {
                    // clear/cancel cached "global" event (see above or below)
                    _globalMouseButton = 0;
                    // ignore. We click() on SDL_MOUSEBUTTONUP
                    break;
                }
                case SDL_MOUSEBUTTONUP: {
                    int button = ev.button.button;
                    int x = ev.button.x;
                    int y = ev.button.y;
                    auto winIt = _windows.find(ev.button.windowID);
                    if (winIt != _windows.end()) {
                        winIt->second->onClick.emit(winIt->second, x, y, button);
                    }
                    break;
                }
                case SDL_MOUSEMOTION: {
                    unsigned buttons = ev.motion.state;
                    int x = ev.motion.x;
                    int y = ev.motion.y;
                    auto winIt = _windows.find(ev.motion.windowID);
                    if (winIt != _windows.end()) {
                        winIt->second->onMouseMove.emit(winIt->second, x, y, buttons);
                    }
                    break;
                }
                case SDL_WINDOWEVENT: {
                    if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        int x = ev.window.data1;
                        int y = ev.window.data2;
                        auto winit = _windows.find(ev.window.windowID);
                        if (winit != _windows.end()) {
                            winit->second->setSize({x,y});
                            destructiveEvent = true;
                        }
                    }
                    else if (ev.window.event == SDL_WINDOWEVENT_CLOSE) {
                        if (_windows.begin() != _windows.end() && _windows.begin()->first == ev.window.windowID) {
                            // Quit application when closing main window
                            // TODO: handle this through a callback
                            printf("Ui: Main window closed\n");
                            return false;
                        }
                        auto winit = _windows.find(ev.window.windowID);
                        if (winit != _windows.end()) {
                            onWindowDestroyed.emit(this,winit->second);
                            delete winit->second;
                            _windows.erase(winit);
                        }
                    }
                    else if (ev.window.event == SDL_WINDOWEVENT_LEAVE) {
                        // NOTE: SDL does not have one cursor per window, which is complete BS
                        auto winit = _windows.find(ev.window.windowID);
                        if (winit != _windows.end()) {
                            winit->second->onMouseLeave.emit(winit->second);
                        }
                    }
                    else if (ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                        // SDL eats the mouse event that was used to get focus,
                        // which is wrong since keyboard focus != mouse focus.
                        // We check for global mouse down on focus and
                        // schedule to fire onClick on global mouse up
                        unsigned button = SDL_GetGlobalMouseState(nullptr, nullptr);
                        if (button) {
                            SDL_Window* win = SDL_GetMouseFocus();
                            if (win) {
                                _globalMouseButton = button;
                            }
                        }
                    }
                    #if 0 // this is not neccessary
                    else if (ev.window.event == SDL_WINDOWEVENT_TAKE_FOCUS) {
                        // focus offered -> grab focus
                        auto winit = _windows.find(ev.window.windowID);
                        if (winit != _windows.end()) {
                            winit->second->grabFocus();
                        }
                    }
                    #endif
                    #if 0
                    else {
                        printf("Ui: window event %d for %d\n", ev.window.event, ev.window.windowID);
                    }
                    #endif
                    break;
                }
                default: {
                    #ifndef NDEBUG
                    printf("Ui: unhandled event %d\n", ev.type);
                    #endif
                }
            }
        }
        t1 = SDL_GetTicks();
        #ifndef VSYNC
        if (destructiveEvent) break; // framebuffer destroyed -> redraw ASAP (unless VSYNC)
        #endif
#if defined __EMSCRIPTEN__ || !defined LIMIT_FPS
    } while (false); // waiting for events makes no sense in a browser context
#else
    } while (FRAME_TIME>_lastRenderDuration && t1-t0+1 < FRAME_TIME-_lastRenderDuration); // TODO: microseconds?
#endif
    
    for (auto win: _windows)
        win.second->render();
    
    uint32_t t2 = SDL_GetTicks();
    uint32_t td = t2-t1;
#if defined LIMIT_FPS && !defined VSYNC && !defined __EMSCRIPTEN__
    {
        // usleep the rest between last frame's timestamp and now to have a constant frame time
        uint64_t timestamp = getMicroTicks();
        uint64_t now = timestamp;
        uint64_t t = 1000000/LIMIT_FPS;
        if (now-_lastFrameMicroTimestamp<t) {
            usleep(t-(now-_lastFrameMicroTimestamp));
        }
        _lastFrameMicroTimestamp = timestamp;
    }
#endif
    _lastRenderDuration = td;
    if (_lastRenderDuration < 1) _lastRenderDuration = 1;
    
    return true;
}

} // namespace
