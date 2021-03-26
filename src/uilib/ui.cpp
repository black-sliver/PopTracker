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
    
    printf("Init SDL...\n");
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
    for (auto win: _windows)
        delete win.second;
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
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            /* handle your event here */
            if (ev.type == SDL_QUIT) return false;
            if (ev.type == SDL_MOUSEBUTTONUP) {
                int button = ev.button.button;
                int x = ev.button.x;
                int y = ev.button.y;
                auto winIt = _windows.find(ev.button.windowID);
                if (winIt != _windows.end()) {
                    winIt->second->onClick.emit(winIt->second, x, y, button);
                }
            }
            else if (ev.type == SDL_MOUSEMOTION) {
                unsigned buttons = ev.motion.state;
                int x = ev.motion.x;
                int y = ev.motion.y;
                auto winIt = _windows.find(ev.motion.windowID);
                if (winIt != _windows.end()) {
                    winIt->second->onMouseMove.emit(winIt->second, x, y, buttons);
                }
            }
            else if (ev.type == SDL_WINDOWEVENT) {
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
            }
    #ifndef NDEBUG
            else {
                printf("Ui: unhandled event %d\n", ev.type);
            }
    #endif
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
