#ifndef _UILIB_BUTTON_H
#define _UILIB_BUTTON_H

#include "label.h"
#include <string>

namespace Ui {

class Button : public Label {
public:
    enum class State {
        NORMAL = 0,
        PRESSED = 1,
        AUTO = -1
    };

    Button(int x, int y, int w, int h, FONT font, const std::string& text);
    virtual ~Button();
    virtual void render(Renderer renderer, int offX, int offY);
    virtual void setText(const std::string& text);
    void setIcon(const void* data, size_t len);
    void setState(State state) { _state = state; }
    bool getPressed() const { return _state == State::AUTO ? _autoState : (bool)_state; }

    static constexpr int ICON_SIZE = 17;

protected:
    int _padding = 4;// TODO: move padding to label?
    State _state = State::AUTO;
    bool _autoState = false;
    SDL_Surface* _iconSurf = nullptr;
    SDL_Texture* _iconTex = nullptr;
    Size _iconSize = {0, 0};
};

} // namespace Ui

#endif // _UILIB_BUTTON_H
