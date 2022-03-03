#ifndef _UILIB_SIMPLECONTAINER_H
#define _UILIB_SIMPLECONTAINER_H

#include "container.h"

namespace Ui {

// 1 child that spans the whole container
class SimpleContainer : public Container {
public:
    SimpleContainer(int x, int y, int w, int h)
        : Container(x,y,w,h)
    {}
    // TODO: overwrite setSize, addChild
    virtual void setSize(Size size) override
    {
        if (size == _size) return;
        Container::setSize(size);
        for (auto child : _children) {
            Size childSize = child->getSize();
            if (child->getHGrow()) childSize.width = _size.width-child->getLeft();
            if (child->getVGrow()) childSize.height = _size.height-child->getTop();
            child->setSize(childSize);
            break; // only the first child for now; TODO: only conatainer-sized widgets?
        }
    }
    virtual void addChild(Widget* child) override
    {
        if (!child) return;
        Container::addChild(child);
        bool fireMin = calculateMinSize(child); // TODO; move this to Container?
        bool fireMax = calculateMaxSize(child);
        if (fireMin) onMinSizeChanged.emit(this);
        if (fireMax) onMaxSizeChanged.emit(this);
    }
    virtual void removeChild(Widget* child) override
    {
        if (!child) return;
        Container::removeChild(child);
        auto oldMin = _minSize; // TODO: move this to Container?
        auto oldMax = _maxSize;
        _minSize = {0,0};
        _maxSize = Size::UNDEFINED;
        for (auto w : _children) {
            calculateMinSize(w);
            calculateMaxSize(w);
        }
        bool fireMin = (_minSize != oldMin);
        bool fireMax = (_maxSize != oldMax);
        if (fireMin) onMinSizeChanged.emit(this);
        if (fireMax) onMaxSizeChanged.emit(this);
    }
    
    Signal<> onMinSizeChanged; // TODO: make split between simplecontainer and container more logical
    Signal<> onMaxSizeChanged;
    
private:
    bool calculateMinSize(Widget* child)
    {
        int minw = child->getLeft() + child->getMinWidth() + child->getMargin().right;
        int minh = child->getTop() + child->getMinHeight() + child->getMargin().bottom;
        Size old = _minSize;
        if (minw>_minSize.width) _minSize.width = minw;
        if (minh>_minSize.height) _minSize.height = minh;
        return (old != _minSize);
    }
    bool calculateMaxSize(Widget* child)
    {
        int maxw = child->getMaxWidth();
        int maxh = child->getMaxHeight();
        if (maxw>=0) maxw += child->getLeft() + child->getMargin().right;
        if (maxh>=0) maxh += child->getTop() + child->getMargin().bottom;
        Size old = _maxSize;
        if (maxw>=0 && maxw<_maxSize.width)  _maxSize.width = maxw;
        if (maxh>=0 && maxh<_maxSize.height) _maxSize.height = maxh;
        if (maxw>=0 && maxw<_minSize.width)  _maxSize.width=_minSize.width;
        if (maxh>=0 && maxh<_minSize.height) _maxSize.height=_minSize.height;
        return (old != _maxSize);
    }
};

} // namespace

#endif // _UILIB_SIMPLECONTAINER_H
