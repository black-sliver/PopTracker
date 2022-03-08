#ifndef _UILIB_CONTAINER_H
#define _UILIB_CONTAINER_H

#include "widget.h"
#include <deque>
#include <algorithm>

#ifndef NDEBUG
#include <typeinfo>
#include <stdio.h>
#endif

namespace Ui {

class Container : public Widget {
public:
    virtual ~Container() {
        clearChildren();
    }
    virtual void addChild(Widget* child) {
        if (!child) return;
        _children.push_back(child);
        if (child->getHGrow()>_hGrow) _hGrow = child->getHGrow();
        if (child->getVGrow()>_vGrow) _vGrow = child->getVGrow();
    }
    virtual void removeChild(Widget* child) {
        if (!child) return;
        if (child == _hoverChild) {
            child->onMouseLeave.emit(child);
            _hoverChild = nullptr;
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
    virtual void clearChildren() {
        if (_hoverChild) {
            _hoverChild->onMouseLeave.emit(_hoverChild);
            _hoverChild = nullptr;
        }
        for (auto& child : _children) {
            delete child;
        }
        _children.clear();
    }
    virtual void raiseChild(Widget* child) {
        if (!child) return;
        for (auto it=_children.begin(); it!=_children.end(); it++) {
            if (*it == child) {
                _children.erase(it);
                _children.push_back(child);
                return;
            }
        }
    }
    virtual void render(Renderer renderer, int offX, int offY) override {
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
        for (auto& child: _children)
            if (child->getVisible())
                child->render(renderer, offX+_pos.left, offY+_pos.top);
    }
    const std::deque<Widget*> getChildren() const { return _children; }

    virtual bool isHover(Widget* w) const override {
        return (w == this || (_hoverChild && _hoverChild->isHover(w)));
    }

    virtual bool isHit(int x, int y) const override {
        if (_backgroundColor.a && Widget::isHit(x, y)) return true;
        x -= _pos.left;
        y -= _pos.top;
        for (const auto& w: _children) {
            if (w->isHit(x, y)) return true;
        }
        return false;
    }

    virtual const Widget* getHit(int x, int y) const override {
        for (const auto& w: _children) {
            if (w->getHit(x - w->getLeft(), y - w->getTop())) return w;
        }
        if (Widget::isHit(x + _pos.left, y + _pos.top)) return this;
        return nullptr;
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
    
    Container(int x=0, int y=0, int w=0, int h=0)
        : Widget(x,y,w,h)
    {
        onClick += { this, [this](void* s, int x, int y, int button) {
            for (auto childIt = _children.rbegin(); childIt != _children.rend(); childIt++) {
                auto& child = *childIt;
                if (child->getVisible() && child->isHit(x, y)) {
                    child->onClick.emit(child, x-child->getLeft(), y-child->getTop(), button);
                    break;
                }
            }
        }};
        onMouseMove += { this, [this](void* s, int x, int y, unsigned buttons) {
            auto oldHoverChild = _hoverChild;
            _hoverChild = nullptr;
            for (auto childIt = _children.rbegin(); childIt != _children.rend(); childIt++) {
                auto& child = *childIt;
                if (child->getVisible() && child->isHit(x, y)) {
                    _hoverChild = child;
                    if (_hoverChild != oldHoverChild) {
                        if (oldHoverChild) oldHoverChild->onMouseLeave.emit(oldHoverChild);
                        _hoverChild->onMouseEnter.emit(_hoverChild, x-child->getLeft(), y-child->getTop(), buttons);
                    }
                    child->onMouseMove.emit(child, x-child->getLeft(), y-child->getTop(), buttons);
                    break;
                }
            }
            if (oldHoverChild && !_hoverChild) oldHoverChild->onMouseLeave.emit(oldHoverChild);
        }};
        onMouseLeave += { this, [this](void* s) {
            auto oldHoverChild = _hoverChild;
            _hoverChild = nullptr;
            if (oldHoverChild) {
                oldHoverChild->onMouseLeave.emit(oldHoverChild);
            }
        }};
        onScroll += { this, [this](void* s, int x, int y, unsigned mod) {
            if (_hoverChild) _hoverChild->onScroll.emit(_hoverChild, x, y, mod);
        }};
    }
};

} // namespace Ui

#endif // _UILIB_CONTAINER_H
