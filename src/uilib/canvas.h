#ifndef _UILIB_CANVAS_H
#define _UILIB_CANVAS_H

#include "simplecontainer.h"

namespace Ui {

class Canvas : public SimpleContainer {
public:
    Canvas(int x, int y, int w, int h)
        : SimpleContainer(x, y, w, h)
    {
    }
    
    virtual void addChild(Widget* child) override
    {
        if (!child) return;
        SimpleContainer::addChild(child);
        child->setMargin({0,0,0,0});
    }
protected:
};

} // namespace Ui

#endif // _UILIB_CANVAS_H

