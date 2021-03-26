#ifndef _UILIB_DOCK_H
#define _UILIB_DOCK_H

#include "container.h"
#include "../core/direction.h"
#include <list>

namespace Ui {

class Dock : public Container {
public:    
    Dock(int x, int y, int w, int h, bool preferHorizontal=false);
    virtual void addChild(Widget* w) override;
    virtual void addChild(Widget* w, Direction dir);
    virtual void removeChild(Widget* w) override;
    virtual void setDock(int index, Direction dir);
    virtual void setSize(Size size) override;
protected:
    void relayout();
    std::list<Direction> _docks;
    bool _preferHorizontal;
};

} // namespace Ui

#endif // _UILIB_DOCK_H
