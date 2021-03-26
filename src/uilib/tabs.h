#ifndef _UILIB_TABS_H
#define _UILIB_TABS_H

// TODO: ideally we would want an HBOX that has the first and last element as
//       Spacers that just grow the same amount to center the buttons
//       for now we just size everything "manually"

#include "container.h"
#include "hbox.h"
#include "button.h"
#include <list>

namespace Ui {

class Tabs : public Container {
public:
    using FONT = Button::FONT;
    Tabs(int x, int y, int w, int h, FONT font);
    virtual ~Tabs();
    
    virtual void addChild(Widget* w) override;
    virtual void removeChild(Widget* w) override;
    virtual void render(Renderer renderer, int offX, int offY) override;
    virtual void setSize(Size size) override;
    virtual void setTabName(int index, const std::string& name);
protected:
    void relayout();
    HBox* _buttonbox;
    std::list<Button*> _buttons;
    FONT _font;
    int _spacing = 0;
    int _tabIndex = 0;
    Widget* _tab = nullptr;
    Button* _tabButton = nullptr;
};

} // namespace Ui

#endif // _UILIB_TABS_H
