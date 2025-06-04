#pragma once

#include <algorithm>
#include <deque>
#include <limits>
#include "label.h" // HAlign, VAlign
#include "widget.h"

#ifndef NDEBUG
#include <typeinfo>
#include <stdio.h>
#endif


namespace Ui {

class Container : public Widget {
public:
    virtual ~Container()
    {
        clearChildren();
    }

    virtual void addChild(Widget* child)
    {
        if (!child) return;
        _children.push_back(child);
        if (child->getHGrow()>_hGrow) _hGrow = child->getHGrow();
        if (child->getVGrow()>_vGrow) _vGrow = child->getVGrow();
    }

    virtual void removeChild(Widget* child)
    {
        if (!child) return;
        if (child == _hoverChild) {
            _hoverChild = nullptr;
            child->onMouseLeave.emit(child);
        }
        _children.erase(std::remove(_children.begin(), _children.end(), child), _children.end());
        if ((_hGrow>0 && child->getHGrow()>=_hGrow) || (_vGrow>0 && child->getVGrow()>=_vGrow)) {
            int oldHGrow = _hGrow; int oldVGrow = _vGrow;
            _hGrow = 0; _vGrow = 0;
            for (auto& tmp : _children) {
                if (tmp->getHGrow()>=_hGrow) _hGrow = tmp->getHGrow();
                if (tmp->getVGrow()>=_vGrow) _vGrow = tmp->getVGrow();
                if (_hGrow == oldHGrow && _vGrow == oldVGrow) break;
            }
        }
    }

    virtual bool hasChild(Widget* child)
    {
        return std::find(_children.begin(), _children.end(), child) != _children.end();
    }

    virtual void clearChildren()
    {
        if (_hoverChild) {
            _hoverChild->onMouseLeave.emit(_hoverChild);
            _hoverChild = nullptr;
        }
        for (auto& child : _children) {
            delete child;
        }
        _children.clear();
    }

    virtual void raiseChild(Widget* child)
    {
        if (!child) return;
        for (auto it=_children.begin(); it!=_children.end(); it++) {
            if (*it == child) {
                _children.erase(it);
                _children.push_back(child);
                return;
            }
        }
    }

    void render(Renderer renderer, int offX, int offY) override
    {
        if (_hAlign == Label::HAlign::LEFT) {
            _renderOffset.left = 0;
        } else {
            int maxX = 0;
            int minX = std::numeric_limits<int>::max();
            for (const auto child : _children) {
                if (child->getLeft() < minX)
                    minX = child->getLeft();
                if (child->getLeft() + child->getWidth() > maxX)
                    maxX = child->getLeft() + child->getWidth();
            }
            if (_hAlign == Label::HAlign::RIGHT) {
                _renderOffset.left = std::max(0, _size.width - (maxX - minX));
            } else {
                _renderOffset.left =  std::max(0, (_size.width - (maxX - minX))/2);
            }
        }

        if (_vAlign == Label::VAlign::TOP) {
            _renderOffset.top = 0;
        } else {
            int maxY = 0;
            int minY = std::numeric_limits<int>::max();
            for (const auto child : _children) {
                if (child->getTop() < minY)
                    minY = child->getTop();
                if (child->getTop() + child->getHeight() > maxY)
                    maxY = child->getTop() + child->getHeight();
            }
            if (_vAlign == Label::VAlign::BOTTOM) {
                _renderOffset.top = std::max(0, _size.height - (maxY - minY));
            } else {
                _renderOffset.top = std::max(0, (_size.height - (maxY - minY))/2);
            }
        }

        if (_backgroundColor.a > 0) {
            const auto& c = _backgroundColor;
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_Rect r = { offX+_pos.left-_margin.left,
                           offY+_pos.top-_margin.top,
                           _size.width+_margin.left+_margin.right,
                           _size.height+_margin.top+_margin.bottom
            };
            SDL_RenderFillRect(renderer, &r);
        }
        // TODO: background image
        offX += _renderOffset.left;
        offY += _renderOffset.top;
        for (auto& child: _children)
            if (child->getVisible())
                child->render(renderer, offX+_pos.left, offY+_pos.top);
    }

    const std::deque<Widget*>& getChildren() const
    {
        return _children;
    }

    bool isHover(Widget* w) const override
    {
        return (w == this || (_hoverChild && _hoverChild->isHover(w)));
    }

    bool isHit(int x, int y) const override
    {
        if (!_mouseInteraction)
            return false;
        if (_backgroundColor.a && Widget::isHit(x, y))
            return true;
        x -= _pos.left + _renderOffset.left;
        y -= _pos.top + _renderOffset.top;
        for (const auto& w: _children) {
            if (w->isHit(x, y))
                return true;
        }
        return false;
    }

    const Widget* getHit(int x, int y) const override
    {
        for (const auto& w: _children) {
            if (w->getHit(x - w->getLeft() - _renderOffset.left,
                          y - w->getTop() - _renderOffset.top))
                return w;
        }
        if (Widget::isHit(x + _pos.left, y + _pos.top))
            return this;
        return nullptr;
    }

    void setHAlign(const Label::HAlign hAlign)
    {
        _hAlign = hAlign;
    }

    void setVAlign(const Label::VAlign vAlign)
    {
        _vAlign = vAlign;
    }

#ifndef NDEBUG
    void dumpTree(size_t depth=0) {
        printf("%s", std::string(depth, ' ').c_str());
        dumpInfo();
        depth += 2;
        for (auto child: _children) {
            Container* c = dynamic_cast<Container*>(child);
            if (c) c->dumpTree(depth);
            else {
                printf("%s", std::string(depth, ' ').c_str());
                child->dumpInfo();
            }
        }
    }
#endif

protected:
    std::deque<Widget*> _children;
    Widget* _hoverChild = nullptr;
    Position _renderOffset = {0, 0};
    Label::HAlign _hAlign = Label::HAlign::LEFT;
    Label::VAlign _vAlign = Label::VAlign::TOP;

    Container(int x=0, int y=0, int w=0, int h=0)
        : Widget(x,y,w,h)
    {
        onClick += { this, [this](void* s, int x, int y, int button) {
            x -= _renderOffset.left;
            y -= _renderOffset.top;
            for (auto childIt = _children.rbegin(); childIt != _children.rend(); childIt++) {
                auto& child = *childIt;
                if (child->getVisible() && child->isHit(x, y)) {
                    child->onClick.emit(child, x-child->getLeft(), y-child->getTop(), button);
                    break;
                }
            }
        }};

        onMouseMove += { this, [this](void* s, int x, int y, unsigned buttons) {
            x -= _renderOffset.left;
            y -= _renderOffset.top;
            auto oldHoverChild = _hoverChild;
            bool match = false;
            for (auto childIt = _children.rbegin(); childIt != _children.rend(); childIt++) {
                Widget* child = *childIt;
                if (child->getVisible() && child->isHit(x, y)) {
                    if (child != oldHoverChild) {
                        if (oldHoverChild) {
                            _hoverChild = nullptr;
                            oldHoverChild->onMouseLeave.emit(oldHoverChild);
                        }
                        if (!hasChild(child)) // the above event could modify the container
                            child = nullptr;
                        _hoverChild = child;
                        if (child)
                            child->onMouseEnter.emit(_hoverChild, x-child->getLeft(), y-child->getTop(), buttons);
                    }
                    if (child) {
                        child->onMouseMove.emit(child, x-child->getLeft(), y-child->getTop(), buttons);
                        match = true;
                        break;
                    }
                }
            }
            if (!match)
                _hoverChild = nullptr;
            if (oldHoverChild && !_hoverChild)
                oldHoverChild->onMouseLeave.emit(oldHoverChild);
        }};

        onMouseLeave += { this, [this](void* s) {
            auto oldHoverChild = _hoverChild;
            if (oldHoverChild) {
                _hoverChild = nullptr;
                oldHoverChild->onMouseLeave.emit(oldHoverChild);
            }
        }};

        onScroll += { this, [this](void* s, int x, int y, unsigned mod) {
            x -= _renderOffset.left;
            y -= _renderOffset.top;
            if (_hoverChild)
                _hoverChild->onScroll.emit(_hoverChild, x, y, mod);
        }};
    }
};

} // namespace Ui
