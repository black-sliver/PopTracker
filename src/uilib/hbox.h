#ifndef _UILIB_HBOX_H
#define _UILIB_HBOX_H

#include "container.h"

namespace Ui {

class HBox : public Container {
protected:
    int _padding=0;
    int _spacing=2;
public:
    HBox(int x, int y, int w, int h)
        : Container(x,y,w,h) {}
    virtual void addChild(Widget* w) override {
        if (!_children.empty()) {
            auto& last = _children.back();
            int lastRight = last->getLeft() + last->getWidth() + last->getMargin().right;
            w->setPosition({lastRight + _spacing + w->getMargin().left, _padding + w->getMargin().top});
            Container::addChild(w);
            _size.width = w->getLeft() + w->getWidth() + w->getMargin().right + _padding;
            _size.height = std::max(_size.height,
                    w->getTop() + w->getHeight() + w->getMargin().bottom + _padding);
        } else {
            w->setPosition({_padding + w->getMargin().left, _padding + w->getMargin().top});
            _size = {w->getLeft() + w->getWidth() + w->getMargin().right + _padding,
                     w->getTop() + w->getHeight() + w->getMargin().bottom + _padding};
            Container::addChild(w);
        }
        calcMinMax();
    }

    virtual ~HBox() {}

    // TODO: removeChild: update maxSize and minSize
    virtual void setSize(Size size) override {
        if (size.height < _minSize.height) {
            // this may happen if container<this widget
            size.height = _minSize.height;
        }
        if (size.width < _minSize.width) {
            // this may happen if container<this widget
            size.width = _minSize.width;
        }
        Container::setSize(size);
        // TODO: move this to relayout and run relayout instead
        for (auto child : _children) {
            child->setTop(_padding + child->getMargin().top);
            child->setHeight(size.height - child->getTop() - child->getMargin().bottom - _padding);
        }
        // FIXME: make this depend on vgrow of *all* chrildren
        if (!_children.empty() && _children.back()->getHGrow()) {
            // FIXME: this code actually gets run for every addChild()
            _children.back()->setWidth(size.width - _children.back()->getLeft() - _padding);
        }
    }
    void relayout() { // TODO: virtual?
        int x=_padding;
        for (auto& child : _children) {
            x += child->getMargin().left;
            child->setTop(child->getMargin().top + _padding);
            child->setLeft(x);
            x += child->getWidth() + child->getMargin().right + _spacing;
        }
        calcMinMax();
    }
    virtual void setPadding(int padding) { _padding = padding; } // TODO: relayout?
    virtual void setSpacing(int spacing) { _spacing = spacing; } // TODO: relayout?

private:
    void calcMinMax() {
        _maxSize = {2*_padding,-1};
        _minSize = {2*_padding,0};
        if (!_children.empty()) {
            // handle spacing
            _minSize.width = 2*_padding - _spacing;
            _maxSize.width = 2*_padding - _spacing;
        }
        for (auto& child : _children) {
            if (child->getMaxHeight()>=0) {
                int newMaxHeight = child->getTop() + child->getMaxHeight() + child->getMargin().bottom + _padding;
                if (_maxSize.height < 0 || newMaxHeight < _maxSize.height) _maxSize.height = newMaxHeight;
            }
            _minSize.height = std::max(_minSize.height,
                    child->getTop() + child->getMinHeight() + child->getMargin().bottom + _padding);
            _minSize.width += child->getMinWidth() + child->getMargin().left + child->getMargin().right + _spacing;
            if (_maxSize.width != -1) {
                if (child->getMaxWidth() == -1)
                    _maxSize.width = -1; // undefined = no max
                else
                    _maxSize.width += child->getMaxWidth() + child->getMargin().left + _spacing;
            }
        }
        if (_maxSize.width >= 0 && _minSize.width>_maxSize.width)
            _maxSize.width = _minSize.width;
        if (_maxSize.height >= 0 && _minSize.height>_maxSize.height)
            _maxSize.height = _minSize.height;
    }
};

} // namespace Ui

#endif // _UILIB_HBOX_H
