#include "dock.h"
#include <map>

namespace Ui {

// NOTE: code below is not perfect. we would like to have vGrow/hGrow everywhere,
//       but dock does not make this easy, so we try to just emulate expected behaviour

Dock::Dock(int x, int y, int w, int h, bool preferHorizontal)
    : Container(x,y,w,h), _preferHorizontal(preferHorizontal)
{}

void Dock::addChild(Widget* w)
{
    addChild(w, Direction::UNDEFINED);
}
void Dock::addChild(Widget* w, Direction dir)
{
    Container::addChild(w);
    _docks.push_back(dir);
    relayout();
}
void Dock::removeChild(Widget* w)
{
    auto dockIt = _docks.begin();
    auto childIt = _children.begin();
    for (;dockIt != _docks.end() && childIt != _children.end(); dockIt++, childIt++)
    {
        if (*childIt == w) {
            _docks.erase(dockIt);
            Container::removeChild(w);
            relayout();
            break;
        }
    }
}
void Dock::setDock(int index, Direction dir)
{
    if (_docks.empty()) return;
    if (index<0) {
        for (auto it = _docks.rbegin(); it != _docks.rend(); it++) {
            index++;
            if (index==0) {
                if (*it != dir) {
                    *it = dir;
                    relayout();
                }
                break;
            }
        }
    } else {
        for (auto it = _docks.begin(); it != _docks.end(); it++) {
            if (index==0) {
                if (*it != dir) {
                    *it = dir;
                    relayout();
                }
                break;
            }
            index--;
        }
    }
}
void Dock::relayout()
{
    // NOTE: this does two things:
    // 1. set hgrow/vgrow and minsize to signal parent based on children and dock options
    // 2. actually layout children based on (current) size
    // TODO: implement this better
    bool isHorizontal = false;
    bool isVertical = false;
    int nUndefined = 0;
    auto dockIt = _docks.begin();
    auto childIt = _children.begin();
    int horizontalMinWidth = 0;
    int verticalMinHeight = 0;
    _minSize = {0,0};
    for (;dockIt != _docks.end() && childIt != _children.end(); dockIt++, childIt++)
    {
        if (*dockIt == Direction::UNDEFINED) {
            nUndefined++;
            continue;
        }
        if (*dockIt == Direction::LEFT || *dockIt == Direction::RIGHT) {
            isHorizontal = true;
            // NOTE: we ignore maxHeight here. widgets will have to handle height>maxHeight
            horizontalMinWidth += (*childIt)->getMinWidth();
            if (_minSize.height<(*childIt)->getMinHeight())
                _minSize.height=(*childIt)->getMinHeight();
            // TODO: maxWidth?
        }
        if (*dockIt == Direction::UP || *dockIt == Direction::DOWN) {
            isVertical = true;
            // NOTE: we ignore maxWidth here. widgets will have to handle width>maxWidth
            verticalMinHeight += (*childIt)->getMinHeight();
            if (_minSize.width<(*childIt)->getMinWidth())
                _minSize.width=(*childIt)->getMinWidth();
            // TODO: maxHeight?
        }
    }
    if (nUndefined) {
        // second pass applies minSize of floaters depending on expected layout
        // NOTE: this is still not perfect
        dockIt = _docks.begin();
        childIt = _children.begin();
        for (;dockIt != _docks.end() && childIt != _children.end(); dockIt++, childIt++)
        {
            if (*dockIt != Direction::UNDEFINED) continue;
            bool layoutHorizontal = (isHorizontal && !isVertical) || (isHorizontal==isVertical && _preferHorizontal);
            if (layoutHorizontal) {
                horizontalMinWidth += (*childIt)->getMinWidth();
                if (_minSize.height<(*childIt)->getMinHeight())
                    _minSize.height=(*childIt)->getMinHeight();
            } else {
                verticalMinHeight += (*childIt)->getMinHeight();
                if (_minSize.width<(*childIt)->getMinWidth())
                    _minSize.width=(*childIt)->getMinWidth();
            }
        }
    }
    if (horizontalMinWidth > _minSize.width) _minSize.width = horizontalMinWidth;
    if (verticalMinHeight > _minSize.height) _minSize.height = verticalMinHeight;
    // TODO: preset to -1 (no restriction) if it stayed at zero?
    if (_maxSize.width>=0 && _maxSize.width<_minSize.width) _maxSize.width = _minSize.width;
    if (_maxSize.height>=0 && _maxSize.height<_minSize.height) _maxSize.height = _minSize.height;
    
    Size newSize = _size || _minSize;
    if (newSize.width != _size.width || newSize.height != _size.height) {
        setSize(newSize);
        return;
    }
    
    if (nUndefined) {
        _hGrow = 1;
        _vGrow = 1;
    } else if (isHorizontal && !isVertical) {
        _hGrow = 0; // actually depends on hGrow of children, but we don't implement this yet
        _vGrow = 1;
    } else if (isVertical && !isHorizontal) {
        _hGrow = 1;
        _vGrow = 0;
    } else {
        // empty
        _hGrow = 0;
        _vGrow = 0;
    }
    
    // actyually layout children
    // actually layout docked children
    int top=0, left=0, bottom=_size.height, right=_size.width;
    dockIt = _docks.begin();
    childIt = _children.begin();
    for (;dockIt != _docks.end() && childIt != _children.end(); dockIt++, childIt++)
    {
        if (*dockIt == Direction::UNDEFINED) continue;
#if 0
        int childHGrow = (*childIt)->getHGrow() ? (*childIt)->getHGrow() : 0;
        int childVGrow = (*childIt)->getVGrow() ? (*childIt)->getVGrow() : 0;
#endif
        if (*dockIt == Direction::LEFT || *dockIt == Direction::RIGHT) {
            (*childIt)->setTop(top);
            (*childIt)->setHeight(bottom-top);
            // keep width as-is unless it's used vbox-like
#if 1 // make last child fill available space
            // TODO: only if hAlignment=strech
            if (!nUndefined && *childIt == _children.back()) {
                (*childIt)->setWidth(right-left);
            }
#   if 1 // work-around until we truly populate autosizing through all levels
            else {
                (*childIt)->setWidth((*childIt)->getMinSize().width);
            }
#   endif
#endif
            if (*dockIt == Direction::LEFT) {
                (*childIt)->setLeft(left);
                left += (*childIt)->getWidth();
            }
            else {
                (*childIt)->setLeft(right - (*childIt)->getWidth());
                right -= (*childIt)->getWidth();
            }
        } else {
            (*childIt)->setLeft(left);
            (*childIt)->setWidth(right-left);
            // keep height as-is
#if 1 // make last child fill available space
            // TODO: only if vAlignment=strech
            if (!nUndefined && *childIt == _children.back()) {
                (*childIt)->setHeight(bottom-top);
            }
#   if 1 // work-around until we truly populate autosizing through all levels
            else {
                (*childIt)->setHeight((*childIt)->getMinSize().height);
            }
#   endif
#endif
            if (*dockIt == Direction::UP) {
                (*childIt)->setTop(top);
                top += (*childIt)->getHeight();
            }
            else {
                (*childIt)->setTop(bottom - (*childIt)->getHeight());
                bottom -= (*childIt)->getHeight();
            }
        }
    }
    // actually layout floating children
    dockIt = _docks.begin();
    childIt = _children.begin();
    Size extraSpace = { (right-left), (bottom-top) };
    for (;dockIt != _docks.end() && childIt != _children.end(); dockIt++, childIt++)
    {
        if (*dockIt != Direction::UNDEFINED) continue;
#if 0
        int childHGrow = (*childIt)->getHGrow() ? (*childIt)->getHGrow() : 1;
        int childVGrow = (*childIt)->getVGrow() ? (*childIt)->getVGrow() : 1;
#endif
        bool layoutHorizontal = (isHorizontal && !isVertical) || (isHorizontal==isVertical && _preferHorizontal);
        if (layoutHorizontal) {
            // layout floating children horizontally
            (*childIt)->setHeight(bottom-top);
            (*childIt)->setTop(top);
            (*childIt)->setWidth(extraSpace.width/nUndefined);
            (*childIt)->setLeft(left);
            left += (*childIt)->getWidth();
        } else {
            // layout floating children vertically
            (*childIt)->setWidth(right-left);
            (*childIt)->setLeft(left);
            (*childIt)->setHeight(extraSpace.height/nUndefined);
            (*childIt)->setTop(top);
            top += (*childIt)->getHeight();
        }
    }
    
}

void Dock::setSize(Size size)
{
    if (size.width < _minSize.width) size.width = _minSize.width;
    if (size.height < _minSize.height) size.height = _minSize.height;
    if (size == _size) return;
    Widget::setSize(size);
    relayout();
}

} // namespace
