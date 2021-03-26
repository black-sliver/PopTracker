#ifndef _UILIB_VBOX_H
#define _UILIB_VBOX_H

#include "container.h"

namespace Ui {

class VBox : public Container {
protected:
    int _margin=0;
    int _spacing=2;
public:
    VBox(int x, int y, int w, int h)
        : Container(x,y,w,h) {}
    virtual void addChild(Widget* w) override {
        if (!_children.empty()) {
            auto& last = _children.back();
            w->setPosition({_margin, last->getTop() + last->getHeight() + _spacing});
            Container::addChild(w);
            setHeight(w->getTop() + w->getHeight() + _margin);
            if (w->getLeft() + w->getWidth() + _margin > _size.width) setWidth(w->getLeft() + w->getWidth() + _margin);
        } else {
            w->setPosition({_margin,_margin});
            setSize({w->getWidth()+2*_margin, w->getHeight()+2*_margin});
            Container::addChild(w);
        }
        // keep track of max and min size
        _maxSize = {-1,2*_margin};
        _minSize = {0,2*_margin};
        if (!_children.empty()) {
            // handle spacing and handle case where first child has different margin
            auto child = _children.front();
            _minSize.height = child->getTop() + _margin - _spacing;
            _maxSize.height = child->getTop() + _margin - _spacing;
        }
        for (auto& child : _children) {
            if (_maxSize.width<0 || _maxSize.width>(child->getLeft()+child->getMaxWidth()+_margin))
                _maxSize.width=child->getLeft()+child->getMaxWidth()+_margin;
            if (child->getLeft()+child->getMinWidth()+_margin>_minSize.width)
                _minSize.width=child->getLeft()+child->getMinWidth()+_margin;
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
            child->setLeft(_margin);
            child->setWidth(size.width-child->getLeft()-_margin);
        }
        // FIXME: make this depend on vgrow of *all* chrildren
        if (!_children.empty() && _children.back()->getVGrow()) {
            // FIXME: this code actually gets run for every addChild()
            _children.back()->setHeight(size.height - _children.back()->getTop()-_margin);
        }
    }
    void relayout() { // TODO: virtual?
        _minSize.height = 0; // TODO: subscribe to children's onMinWidthChanged instead
        int y = _margin;
        for (auto& child : _children) {
            child->setTop(y);
            y += child->getHeight() + _spacing;
            _minSize.height += child->getMinHeight() + _spacing;
        }
        if (_minSize.height>0) _minSize.height += 2*_margin - _spacing;
    }
    virtual void setMargin(int margin) { _margin = margin; } // TODO: relayout?
    virtual void setSpacing(int spacing) { _spacing = spacing; } // TODO: relayout?
};

} // namespace Ui

#endif // _UILIB_VBOX_H
