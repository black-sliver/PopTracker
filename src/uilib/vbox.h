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
            int lastBottom = last->getTop() + last->getHeight() + last->getMargin().bottom;
            w->setPosition({_padding + w->getMargin().left, lastBottom + _spacing + w->getMargin().top});
            Container::addChild(w);
            _size.height = w->getTop() + w->getHeight() + w->getMargin().bottom + _padding;
            _size.width = std::max(_size.width,
                    w->getLeft() + w->getWidth() + w->getMargin().right + _padding);
        } else {
            w->setPosition({_padding + w->getMargin().left, _padding + w->getMargin().top});
            _size = {w->getLeft() + w->getWidth() + w->getMargin().right + _padding,
                     w->getTop() + w->getHeight() + w->getMargin().bottom + _padding};
            Container::addChild(w);
        }
        calcMinMax();
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
            child->setLeft(_padding + child->getMargin().left);
            child->setWidth(size.width - child->getLeft() - child->getMargin().right - _padding);
        }
        // FIXME: make this depend on vgrow of *all* chrildren
        if (!_children.empty() && _children.back()->getVGrow()) {
            // FIXME: this code actually gets run for every addChild()
            _children.back()->setHeight(size.height - _children.back()->getTop() - _padding);
        }
    }
    void relayout() { // TODO: virtual?
        int y = _padding;
        for (auto& child : _children) {
            y += child->getMargin().top;
            child->setLeft(child->getMargin().left + _padding);
            child->setTop(y);
            y += std::max(0, child->getHeight()) + child->getMargin().bottom + _spacing;
        }
        calcMinMax();
    }
    virtual void setPadding(int padding) { _padding = padding; } // TODO: relayout?
    virtual void setSpacing(int spacing) { _spacing = spacing; } // TODO: relayout?

private:
    void calcMinMax() {
        _maxSize = {-1,2*_padding};
        _minSize = {0,2*_padding};
        if (!_children.empty()) {
            // handle spacing
            _minSize.height = 2*_padding - _spacing;
            _maxSize.height = 2*_padding - _spacing;
        }
        for (auto& child : _children) {
            if (child->getMaxWidth()>=0) {
                int newMaxWidth = child->getLeft() + child->getMaxWidth() + child->getMargin().right + _padding;
                if (_maxSize.width < 0 || newMaxWidth < _maxSize.width) _maxSize.width = newMaxWidth;
            }
            _minSize.width = std::max(_minSize.width,
                    child->getLeft() + child->getMinWidth() + child->getMargin().right + _padding);
            _minSize.height += child->getMinHeight() + child->getMargin().top + child->getMargin().bottom + _spacing;
            if (_maxSize.height != -1) {
                if (child->getMaxHeight() == -1)
                    _maxSize.height = -1; // undefined = no max
                else
                    _maxSize.height += child->getMaxHeight() + child->getMargin().top + _spacing;
            }
        }
        if (_maxSize.width >= 0 && _minSize.width>_maxSize.width)
            _maxSize.width = _minSize.width;
        if (_maxSize.height >= 0 && _minSize.height>_maxSize.height)
            _maxSize.height = _minSize.height;
    }
};

} // namespace Ui

#endif // _UILIB_VBOX_H
