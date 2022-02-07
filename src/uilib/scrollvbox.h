#ifndef _UILIB_SCROLLVBOX_H
#define _UILIB_SCROLLVBOX_H

#include "container.h"

namespace Ui {

class ScrollVBox : public Container {
protected:
    int _margin=0;
    int _spacing=2;
public:
    ScrollVBox(int x, int y, int w, int h)
        : Container(x,y,w,h)
    {
        onScroll += {this, [this](void*, int x, int y, unsigned mod) {
            scrollBy(x, y);
        }};
    }

    virtual void addChild(Widget* w) override
    {
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
        for (auto& child : _children) {
            if (_maxSize.width<0 || _maxSize.width>(child->getLeft()+child->getMaxWidth()+_margin))
                _maxSize.width=child->getLeft()+child->getMaxWidth()+_margin;
            if (child->getLeft()+child->getMinWidth()+_margin>_minSize.width)
                _minSize.width=child->getLeft()+child->getMinWidth()+_margin;
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
        relayout(); // required to update scroll position
    }

    // TODO: removeChild: update maxSize and minSize

    virtual void setSize(Size size) override
    {
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
            int h = size.height - _children.back()->getTop()-_margin;
            if (h>0) _children.back()->setHeight(h);
        }
        relayout(); // required to update scroll position + max scroll
    }

    void relayout(bool isFixup=false) // TODO: virtual?
    {
        if (_scrollY > 0) _scrollY = 0;
        else if (_children.empty()) _scrollY = 0;

        int y = _margin + _scrollY;
        for (auto& child : _children) {
            child->setTop(y);
            y += child->getHeight() + _spacing;
        }

        // recalculate max scroll, and fix up scrolling if outside of window
        if (!_children.empty() && !isFixup) {
            int oldScrollY = _scrollY;
            const auto& last = _children.back();
            int bot = last->getTop() + last->getHeight();
            _scrollMaxY = _scrollY - (bot - _size.height);
            if (_scrollMaxY > 0) _scrollMaxY = 0;
            if (_scrollY < _scrollMaxY) _scrollY = _scrollMaxY;
            if (_scrollY > 0) _scrollY = 0;
            if (oldScrollY != _scrollY) relayout(true);
        } else if (!isFixup) {
            _scrollMaxY = 0;
        }

        _minSize.height = 2*_margin;
    }

    virtual void setMargin(int margin) { _margin = margin; } // TODO: relayout?
    virtual void setSpacing(int spacing) { _spacing = spacing; } // TODO: relayout?

    virtual void scrollBy(int x, int y)
    {
        if (y==0) return;
        int oldScrollY = _scrollY;
        _scrollY += y;
        if (_scrollY > 0) _scrollY = 0;
        else if (_children.empty()) _scrollY = 0;
        else if (_scrollY < _scrollMaxY) _scrollY = _scrollMaxY;
        if (_scrollY != oldScrollY) relayout();
    }

    virtual void render(Renderer renderer, int offX, int offY) override
    {
        // children
        Container::render(renderer, offX, offY);
        // scroll bar/position
        if (_scrollMaxY < 0 && _size.height > 0) {
            int x = _pos.left + _size.width - 3;
            int y = _pos.top;
            int w = 2;
            int h = _size.height;
            int barh = (h * _size.height) / (_size.height - _scrollMaxY);
            if (barh >= h) barh = h-1;
            if (barh < 2) barh = 2;
            int bary = y + (h - barh)*_scrollY/_scrollMaxY; // TODO
            SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x80);
            SDL_Rect r1 = { offX+x, offY+y, w, h };
            SDL_RenderFillRect(renderer, &r1);
            SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0x80);
            SDL_Rect r2 = { offX+x, offY+bary, w, barh };
            SDL_RenderFillRect(renderer, &r2);
        }
    }

protected:
    int _scrollMaxY = 0;
    int _scrollY = 0;
};

} // namespace Ui

#endif // _UILIB_VBOX_H
