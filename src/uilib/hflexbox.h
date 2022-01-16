#ifndef _UILIB_HFLEXBOX_H
#define _UILIB_HFLEXBOX_H

#include "container.h"
#include <list>

// HFlexBox is like HBox (horizontal array) but may break into multiple rows when it gets too wide

namespace Ui {

class HFlexBox : public Container {
public:
    enum class HAlign {
        LEFT = -1,
        CENTER = 0,
        RIGHT = 1
    };
    
protected:
    int _margin=2;
    int _spacing=2;
    HAlign _halign;

public:
    HFlexBox(int x, int y, int w, int h, HAlign halign)
        : Container(x,y,w,h), _halign(halign) {}
    virtual void addChild(Widget* w) override {
        Container::addChild(w);
        relayout();
    }
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
        if (size == _size) return;
        if (size.width == _size.width) {
            Container::setSize(size);
        } else {
            Container::setSize(size);
            relayout();
        }
    }
    void alignRow(const std::list<Widget*>& row)
    {
        // backtrack through previous row to align center/right
        if (row.empty()) return;
        if (_halign == HAlign::CENTER || _halign == HAlign::RIGHT) {
            int usedW = row.back()->getLeft() + row.back()->getWidth() - _margin;
            int offX = (_size.width - 2*_margin) - usedW;
            if (_halign == HAlign::CENTER) offX/=2;
            for (auto& w : row)
                w->setLeft(w->getLeft()+offX);
        }
    }
    void relayout() { // TODO: virtual?
        // TODO: move actual relayout to render stage?
        _minSize.width = 0; // TODO: subscribe to children's onMinWidthChanged instead
        _minSize.height = 0;
        int x=_margin;
        int y=_margin;
        int nextY = y + _spacing;
        std::list<Widget*> row;
        for (auto& child : _children) {
            if (x>0 && x+child->getWidth()+_margin>_size.width) {
                alignRow(row);
                row.clear();
                y = nextY;
                x = 0;
            }
            child->setTop(y);
            child->setLeft(x);
            row.push_back(child);
            x += child->getWidth() + _spacing;
            if (y+child->getHeight()+_spacing > nextY)
                nextY = y+child->getHeight()+_spacing;
            if (child->getMinWidth() > _minSize.width)
                _minSize.width = child->getMinWidth();
        }
        alignRow(row);
        _minSize.height = nextY - _margin - _spacing;
        if (_minSize.width>0) _minSize.width += 2*_margin;
        if (_minSize.height>0) _minSize.height += 2*_margin;
        _size.height = _minSize.height;
    }
    virtual void setMargin(int margin) { _margin = margin; } // TODO: relayout?
    virtual void setSpacing(int spacing) { _spacing = spacing; } // TODO: relayout?
};

} // namespace Ui

#endif // _UILIB_HFLEXBOX_H
