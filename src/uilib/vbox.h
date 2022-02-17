#ifndef _UILIB_VBOX_H
#define _UILIB_VBOX_H

#include "container.h"

namespace Ui {

class VBox : public Container {
protected:
    int _padding=0;
    int _spacing=2;
public:
    VBox(int x, int y, int w, int h)
        : Container(x,y,w,h) {}
    virtual void addChild(Widget* w) override {
        if (!_children.empty()) {
            auto& last = _children.back();
            w->setPosition({_padding, last->getTop() + last->getHeight() + _spacing});
            Container::addChild(w);
            setHeight(w->getTop() + w->getHeight() + _padding);
            if (w->getLeft() + w->getWidth() + _padding > _size.width) setWidth(w->getLeft() + w->getWidth() + _padding);
        } else {
            w->setPosition({_padding,_padding});
            setSize({w->getWidth()+2*_padding, w->getHeight()+2*_padding});
            Container::addChild(w);
        }
        // keep track of max and min size
        _maxSize = {-1,2*_padding};
        _minSize = {0,2*_padding};
        if (!_children.empty()) {
            // handle spacing and handle case where first child has different margin
            auto child = _children.front();
            _minSize.height = child->getTop() + _padding - _spacing;
            _maxSize.height = child->getTop() + _padding - _spacing;
        }
        for (auto& child : _children) {
            if (_maxSize.width<0 || _maxSize.width>(child->getLeft()+child->getMaxWidth()+_padding))
                _maxSize.width=child->getLeft()+child->getMaxWidth()+_padding;
            if (child->getLeft()+child->getMinWidth()+_padding>_minSize.width)
                _minSize.width=child->getLeft()+child->getMinWidth()+_padding;
            _minSize.height += child->getMinHeight()+_spacing;
            if (_maxSize.height != -1) {
                if (child->getMaxHeight() == -1)
                    _maxSize.height = -1; // undefined = no max
                else
                    _maxSize.height += child->getMaxHeight()+_spacing;
            }
        }
        if (_maxSize.width >= 0 && _minSize.width>_maxSize.width)
            _maxSize.width = _minSize.width;
        if (_maxSize.height >= 0 && _minSize.height>_maxSize.height)
            _maxSize.height = _minSize.height;
    }
    // TODO: removeChild: update maxSize and minSize
    virtual void setSize(Size size) override {
        if (size.width < _minSize.width) {
            // this may happen if container<this widget
            size.width = _minSize.width;
        }
        if (size.height < _minSize.height) {
            // this may happen if container<this widget
            size.height = _minSize.height;
        }
        Container::setSize(size);
        // TODO: move this to relayout and run relayout instead
        for (auto child : _children) {
            child->setLeft(_padding);
            child->setWidth(size.width-child->getLeft()-_padding);
        }
        // FIXME: make this depend on vgrow of *all* chrildren
        if (!_children.empty() && _children.back()->getVGrow()) {
            // FIXME: this code actually gets run for every addChild()
            _children.back()->setHeight(size.height - _children.back()->getTop()-_padding);
        }
    }
    void relayout() { // TODO: virtual?
        _minSize.height = 0; // TODO: subscribe to children's onMinWidthChanged instead
        int y = _padding;
        for (auto& child : _children) {
            child->setTop(y);
            y += child->getHeight() + _spacing;
            _minSize.height += child->getMinHeight() + _spacing;
        }
        if (_minSize.height>0) _minSize.height += 2*_padding - _spacing;
    }
    virtual void setPadding(int padding) { _padding = padding; } // TODO: relayout?
    virtual void setSpacing(int spacing) { _spacing = spacing; } // TODO: relayout?
};

} // namespace Ui

#endif // _UILIB_VBOX_H
