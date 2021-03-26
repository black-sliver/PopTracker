#ifndef _APP_H
#define _APP_H

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


class App {
protected:
    virtual bool start() = 0;
    virtual bool frame() = 0;

#ifndef __EMSCRIPTEN__
public:
    int run() { if (!start()) return 1; while(frame()){}; return 0; }
#else
private:
    static App* _emInstance;
    static int emRun(App* instance) {
        if (!instance->start()) return 1;
        _emInstance = instance;
        emscripten_set_main_loop(emFrame, 0, true); // this will not return
        return 0;
    }
    static void emStop(App* instance) {
        if (_emInstance == instance) {
            _emInstance = nullptr; 
            emscripten_cancel_main_loop();
        }
    }
    static void emFrame() {
        if (_emInstance) _emInstance->frame();
    }
public:
    int run() { return emRun(this); }
    virtual ~App() { emStop(this); }
#endif
};

#endif // _APP_H
