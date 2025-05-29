#pragma once

// TODO: ideally we would want an HBOX that has the first and last element as
//       Spacers that just grow the same amount to center the buttons
//       for now we just size everything "manually"

#include <vector>
#include "button.h"
#include "container.h"
#include "hflexbox.h"


namespace Ui {

class Tabs : public Container {
public:
    using FONT = Button::FONT;
    Tabs(int x, int y, int w, int h, FONT font);
    ~Tabs() override;

    void addChild(Widget* w) override;
    void removeChild(Widget* w) override;
    void render(Renderer renderer, int offX, int offY) override;
    void setSize(Size size) override;
    virtual void setTabName(int index, const std::string& name);
    virtual void setTabIcon(int index, const void *data, size_t len);
    virtual bool setActiveTab(const std::string& name);
    virtual bool setActiveTab(int index);
    virtual const std::string& getActiveTabName() const;

    /// reserves memory for the tabs
    void reserve(size_t size);

    bool isHit(int x, int y) const override {
        return _buttonbox->isHit(x - _pos.left, y - _pos.top) || Container::isHit(x, y);
    }

protected:
    void relayout();
    HFlexBox* _buttonbox;
    std::vector<Button*> _buttons;
    FONT _font;
    int _spacing = 0;
    int _tabIndex = 0;
    Widget* _tab = nullptr;
    Button* _tabButton = nullptr;
};

} // namespace Ui
