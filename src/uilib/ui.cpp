#include "ui.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <unistd.h>
#include <stdint.h>
#include "../core/fileutil.h"
#include "droptype.h"
#include "timer.h"


#if defined __LINUX__ || defined __FREEBSD__ || defined __OPENBSD__ || defined __NETBSD__
#   define UI_HAS_CONTINUOUS_RESIZE // for X we get continuous resize events
#endif
#ifndef __WIN32__
#   define UI_SINGLE_THREADED_PUMP_EVENTS // windows receives all events in the main thread
#endif
#if !defined UI_HAS_CONTINUOUS_RESIZE && !defined UI_SINGLE_THREADED_PUMP_EVENTS
    // determine if we need to use the mutex for polled events
#   define EVENT_LOCK(self) std::unique_lock<std::mutex> __eventLock(self->_eventMutex)
#   define EVENT_UNLOCK(self) __eventLock.unlock()
#else
    // no locking required -> nops
#   define EVENT_LOCK(self)
#   define EVENT_UNLOCK(self)
#endif


namespace Ui {

Ui::Ui(const char *name, bool fallbackRenderer)
{
    _name = name;
    _fallbackRenderer = fallbackRenderer;
    
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

    if (_fallbackRenderer) {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
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

#ifndef UI_HAS_CONTINUOUS_RESIZE
    // we need to hook into event handling to get continuous resize events
    // TODO: AddEventWatch?
    SDL_SetEventFilter(Ui::eventFilter, this);
#endif

    printf("Ui: Available renderers: ");
    int count = 0;
    for (int i=0; i<SDL_GetNumRenderDrivers(); i++) {
        SDL_RendererInfo info;
        if (SDL_GetRenderDriverInfo(i, &info) == 0) {
            printf("%s%s%s", count ? ", " : "", info.name,
                               (info.flags & SDL_RENDERER_ACCELERATED) ? " (hw)" : "");
            count++;
        }
    }
    printf(count ? "\n" : "None\n");
}

Ui::~Ui()
{
    printf("Ui: Destroying UI...\n");
    for (auto win: _windows)
        delete win.second;
    printf("Ui: Destroying SDL...\n");
#ifndef UI_HAS_CONTINUOUS_RESIZE
    // see above
    // TODO: DelEventWatch?
    SDL_SetEventFilter(nullptr, nullptr);
#endif
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

void Ui::addHotkey(const Ui::Hotkey& hotkey)
{
    _hotkeys.push_back(hotkey);
}

void Ui::addHotkey(Ui::Hotkey&& hotkey)
{
    _hotkeys.push_back(hotkey);
}

int Ui::eventFilter(void* userdata, SDL_Event *ev)
{
    Ui* ui = (Ui*)userdata;
    if (ev->type == SDL_WINDOWEVENT) {
        if (ev->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            if (!ui->_eventMutex.try_lock()) return 1; // already handling events, skip resize / push to list instead
            int x = ev->window.data1;
            int y = ev->window.data2;
            auto winit = ui->_windows.find(ev->window.windowID);
            if (winit != ui->_windows.end()) {
                // NOTE: calls below may push new events, looping is disabled using the try_lock above
                winit->second->setSize({x,y});
                winit->second->render();
            }
            ui->_eventMutex.unlock();
        }
    }
    return 1; // add to queue
}

bool Ui::render()
{
    // FPS limiter:
    // stay as long in the event loop as possible. sleep to switch tasks.
    // if not using vsync redraw ASAP for destructive events
    // browser context/emscripten: similar to vsync, but waiting is bad
    
    if (_fpsLimit < 1) {
        for (const auto& pair : _windows) {
            if (!pair.second->isAccelerated()) {
                _fpsLimit = _softwareFpsLimit;
                break;
            }
        }
        if (_fpsLimit < 1) {
            _fpsLimit = _hardwareFpsLimit;
        }
        if (_fpsLimit < 1) {
            _fpsLimit = DEFAULT_FPS_LIMIT;
        }
    }

    #define FRAME_TIME (1000/_fpsLimit) // TODO: microseconds
    
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
                    EVENT_LOCK(this);
                    int button = ev.button.button;
                    int x = ev.button.x;
                    int y = ev.button.y;
                    auto winIt = _windows.find(ev.button.windowID);
                    if (winIt != _windows.end()) {
                        winIt->second->onClick.emit(winIt->second, x, y, button);
                    }
                    EVENT_UNLOCK(this);
                    break;
                }
                case SDL_MOUSEMOTION: {
                    EVENT_LOCK(this);
                    unsigned buttons = ev.motion.state;
                    int x = ev.motion.x;
                    int y = ev.motion.y;
                    auto winIt = _windows.find(ev.motion.windowID);
                    if (winIt != _windows.end()) {
                        winIt->second->onMouseMove.emit(winIt->second, x, y, buttons);
                    }
                    EVENT_UNLOCK(this);
                    break;
                }
                case SDL_MOUSEWHEEL: {
                    EVENT_LOCK(this);
                    int x = ev.wheel.x * 48;
                    int y = ev.wheel.y * 48;
                    unsigned mod = 0;
                    auto winIt = _windows.find(ev.motion.windowID);
                    if (winIt != _windows.end()) {
                        winIt->second->onScroll.emit(winIt->second, x, y, mod);
                    }
                    EVENT_UNLOCK(this);
                    break;
                }
                case SDL_KEYDOWN: {
                    EVENT_LOCK(this);
                    if (!ev.key.repeat) {
                        int key = (int)ev.key.keysym.sym;
                        const uint16_t mask = KMOD_CTRL | KMOD_SHIFT | KMOD_GUI;
                        int mod = (int)(ev.key.keysym.mod & mask);
                        for (const auto& hotkey: _hotkeys) {
                            if (key == hotkey.key && mod == hotkey.mod) {
                                onHotkey.emit(this, hotkey);
                                break;
                            }
                            if (hotkey.mod & ~mask) {
                                fprintf(stderr, "Invalid hotkey %d+%d: modifier masked out\n",
                                    hotkey.key, hotkey.mod);
                            }
                        }
                    }
                    EVENT_UNLOCK(this);
                    break;
                }
                case SDL_WINDOWEVENT: {
                    if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        EVENT_LOCK(this);
#if 0
                        // NOTE: SDL should merge resizes, we may still detect and warn if multiple are in the queue
                        SDL_Event next[4+4+1];
                        int n = SDL_PeepEvents(next, sizeof(next)/sizeof(*next),
                                SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
                        int more = 0;
                        for (int i=0; i<n; i++) {
                            if (next[i].type == ev.type &&
                                    next[i].window.event == ev.window.event &&
                                    next[i].window.windowID == ev.window.windowID) {
                                more++;
                            }
                        }
                        if (more) {
                            fprintf(stderr, "WARNING: %d extra resizes for the same window scheduled!\n", more);
                        }
#endif
                        int x = ev.window.data1;
                        int y = ev.window.data2;
                        auto winit = _windows.find(ev.window.windowID);
                        if (winit != _windows.end()) {
                            winit->second->setSize({x,y});
                            destructiveEvent = true;
                        }
                        EVENT_UNLOCK(this);
                    }
                    else if (ev.window.event == SDL_WINDOWEVENT_CLOSE) {
                        EVENT_LOCK(this);
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
                        EVENT_UNLOCK(this);
                    }
                    else if (ev.window.event == SDL_WINDOWEVENT_LEAVE) {
                        // NOTE: SDL does not have one cursor per window, which is complete BS
                        EVENT_LOCK(this);
                        auto winit = _windows.find(ev.window.windowID);
                        if (winit != _windows.end()) {
                            winit->second->onMouseLeave.emit(winit->second);
                        }
                        EVENT_UNLOCK(this);
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
                        EVENT_LOCK_GUARD(this);
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
                case SDL_DROPBEGIN: {
                    // this event is more or less useless:
                    // * it fires on drop, not on drag over
                    // * it does not contain file/text/mime
                    if (ev.drop.file) free(ev.drop.file);
                    break;
                }
                case SDL_DROPFILE: {
                    if (ev.drop.file) {
                        EVENT_LOCK(this);
                        auto winit = _windows.find(ev.drop.windowID);
                        if (winit != _windows.end()) {
                            bool isDir = dirExists(ev.drop.file);
                            winit->second->onDrop.emit(winit->second, 0, 0,
                                    isDir ? DropType::DIR : DropType::FILE,
                                    ev.drop.file);
                        }
                        EVENT_UNLOCK(this);
                        free(ev.drop.file);
                    }
                    break;
                }
                case SDL_DROPTEXT: {
                    if (ev.drop.file) {
                        EVENT_LOCK(this);
                        auto winit = _windows.find(ev.drop.windowID);
                        if (winit != _windows.end()) {
                            winit->second->onDrop.emit(winit->second, 0, 0,
                                    DropType::TEXT,
                                    ev.drop.file);
                        }
                        EVENT_UNLOCK(this);
                        free(ev.drop.file);
                    }
                    break;
                }
                case SDL_DROPCOMPLETE: {
                    if (ev.drop.file) free(ev.drop.file);
                    break;
                }
                #ifndef NDEBUG
                case SDL_KEYMAPCHANGED: { // bogus event
                    break;
                }
                default: {
                    printf("Ui: unhandled event %d\n", ev.type);
                }
                #endif
            }
        }
        t1 = SDL_GetTicks();
        #ifndef VSYNC
        if (destructiveEvent) break; // framebuffer destroyed -> redraw ASAP (unless VSYNC)
        #endif
#if defined __EMSCRIPTEN__
    } while (false); // waiting for events makes no sense in a browser context
#else
    } while (_fpsLimit && (FRAME_TIME>_lastRenderDuration && t1-t0+1 < FRAME_TIME-_lastRenderDuration)); // TODO: microseconds?
#endif
    
    {
        EVENT_LOCK(this);
        for (auto win: _windows)
            win.second->render();
        EVENT_UNLOCK(this);
    }

    uint32_t t2 = SDL_GetTicks();
    uint32_t td = t2-t1;
#if !defined VSYNC && !defined __EMSCRIPTEN__
    if (_fpsLimit)
    {
        // usleep the rest between last frame's timestamp and now to have a constant frame time
        uint64_t timestamp = getMicroTicks();
        uint64_t now = timestamp;
        uint64_t t = 1000000/_fpsLimit;
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

void Ui::makeWindowVisible(Position& pos, const Size& size) const
{
    int numDisplays = SDL_GetNumVideoDisplays();
    Position closest;
    for (int i=0; i<numDisplays; i++) {
        SDL_Rect rect;
        if (SDL_GetDisplayBounds(i, &rect) != 0)
            continue;
        // detect if title bar is visible
        auto left = pos.left;
        auto right = pos.left + size.width;
        auto top = pos.top;
#ifndef __LINUX__
        // only care about (guesstimated) title bar
        auto bottom = pos.top + 32;
#else
        // any corner is fine (alt+drag)
        auto bottom = pos.top + size.height;
#endif
        bool xOK = left < rect.x + rect.w && right > rect.x;
        bool yOK = top < rect.y + rect.h && bottom > rect.y;
        if (xOK && yOK)
            return; // visible
        Position possiblePos = { xOK ? pos.left : rect.x, yOK ? pos.top : rect.y };
        if (closest == Position::UNDEFINED)
            closest = possiblePos;
        else {
            auto dPrev = abs(closest.left - pos.left) + abs(closest.top - pos.top);
            auto dNew = abs(possiblePos.left - pos.left) + abs(possiblePos.top - pos.top);
            if (dNew < dPrev)
                closest = possiblePos;
        }
    }
    if (closest != Position::UNDEFINED) {
        pos.left = closest.left;
        pos.top = closest.top;
    }
}

} // namespace
