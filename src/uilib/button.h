#ifndef _UILIB_BUTTON_H
#define _UILIB_BUTTON_H

#include "label.h"
#include <string>

namespace Ui {

class Button : public Label {
public:
    Button(int x, int y, int w, int h, FONT font, const std::string& text);
    virtual void render(Renderer renderer, int offX, int offY);
    virtual void setText(const std::string& text);
    enum class State {
        NORMAL = 0,
        PRESSED = 1,
        AUTO = -1
    };
    void setState(State state) { _state = state; }
    bool getPressed() const { return _state == State::AUTO ? _autoState : (bool)_state; }
protected:
    int _padding = 4;// TODO: move padding to label?
    State _state = State::AUTO;
    bool _autoState = false;
};

} // namespace Ui

#endif // _UILIB_BUTTON_H
