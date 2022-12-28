#ifndef _UILIB_UI_H
#define _UILIB_UI_H

#include "window.h"
#include <map>
#include <string>
#include <mutex>
#include <list>

#define DEFAULT_FPS_LIMIT 120

namespace Ui {

class Ui {
public:
    struct Hotkey {
        int id;
        int key;
        int mod;
    };

protected:
    std::map<Window::ID,Window*> _windows;
    std::string _name;
    unsigned _lastRenderDuration = 0;
    uint64_t _lastFrameMicroTimestamp = 0;
    unsigned _globalMouseButton = 0;
    bool _fallbackRenderer = false;
    unsigned _fpsLimit = DEFAULT_FPS_LIMIT;

    std::mutex _eventMutex;
    static int eventFilter(void *userdata, SDL_Event *event);

    std::list<Hotkey> _hotkeys;

public:
    Ui(const char *name, bool fallbackRenderer);
    virtual ~Ui();
    template <class T>
    T *createWindow(const char *title, SDL_Surface* icon=nullptr, const Position& pos={-1,-1}, const Size& size={0,0})
    {
        T *win = new T(title,icon,pos,size);
        this->_windows[win->getID()] = win;
        return win;
    }
    void destroyWindow(Window *win);
    bool render();

    void setFPSLimit(unsigned fps) { _fpsLimit = fps; }

    void addHotkey(const Hotkey&);
    void addHotkey(Hotkey&&);

    Signal<Window*> onWindowDestroyed;
    Signal<const Hotkey&> onHotkey;
};

} // namespace Ui

#endif // _UILIB_UI_H
