#ifndef _UILIB_WIDGET_H
#define _UILIB_WIDGET_H

#include <SDL2/SDL.h>
#include "../core/signal.h"
#include "size.h"
#include "position.h"
#include "spacing.h"
#include "cursor.h"
#include <string>

#ifndef NDEBUG
#include <typeinfo>
#include <stdio.h>
#endif

namespace Ui {

using Renderer = SDL_Renderer*;

enum MouseButton { // TODO: enum class
    BUTTON_LEFT = SDL_BUTTON_LEFT,
    BUTTON_MIDDLE = SDL_BUTTON_MIDDLE,
    BUTTON_RIGHT = SDL_BUTTON_RIGHT,
    BUTTON_BACK = SDL_BUTTON_X1,
    BUTTON_FORWARD = SDL_BUTTON_X2,
};

class Widget {
public:
    struct Color { // TODO: move this out of Widget?
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
        constexpr Color(uint8_t R, uint8_t G, uint8_t B, uint8_t A=0xff) : r(R), g(G), b(B), a(A) {}
        constexpr Color() : r(0),g(0),b(0),a(0) {}
        Color(const std::string& s) {
            size_t p = 0;
            if (s[0] == '#') p++;
            if (s.length()-p == 6) { // RRGGBB
                r = hex(s[p+0],s[p+1]);
                g = hex(s[p+2],s[p+3]);
                b = hex(s[p+4],s[p+5]);
                a = 0xff;
            }
            else if (s.length()-p == 3) { // RGB -> RRGGBB
                r = hex(s[p+0],s[p+0]);
                g = hex(s[p+1],s[p+1]);
                b = hex(s[p+2],s[p+2]);
                a = 0xff;
            }
            else if (s.length()-p == 8) { // AARRGGBB
                a = hex(s[p+0],s[p+1]);
                r = hex(s[p+2],s[p+3]);
                g = hex(s[p+4],s[p+5]);
                b = hex(s[p+6],s[p+7]);
            }
            else if (s.length()-p == 4) { // ARGB -> AARRGGBB
                a = hex(s[p+0],s[p+0]);
                r = hex(s[p+1],s[p+1]);
                g = hex(s[p+2],s[p+2]);
                b = hex(s[p+3],s[p+3]);
            }
            else {
                r = 0; g = 0; b = 0; a = 0;
            }
        }
        bool operator==(const Color& other) const { return other.r==r && other.g==g && other.b==b && other.a==a; }
        bool operator!=(const Color& other) const { return ! (*this == other); }

    private:
        uint8_t hex(char c) {
            if (c>='0' && c<='9') return (uint8_t)(c-'0');
            if (c>='a' && c<='f') return (uint8_t)(c-'a'+0x0a);
            if (c>='A' && c<='F') return (uint8_t)(c-'A'+0x0a);
            return 0;
        }
        uint8_t hex(char hi, char lo) {
            return hex(hi)<<4 | hex(lo);
        }
    };
        
protected:
    Widget(int left, int top, int width, int height)
        : _pos(left,top), _size(width,height)
    {
        connectUpdateCursor();
    }
    Widget()
        : _pos(0,0), _size(0,0)
    {
        connectUpdateCursor();
    }
    void connectUpdateCursor()
    {
        onMouseEnter += { this, [this](void*,int,int,unsigned){
            if (_cursor) {
                SDL_SetCursor(_cursor);
                SDL_PumpEvents(); // talk to WM
            }
        }};
        onMouseLeave += { this, [this](void*) {
            if (_cursor) {
                SDL_SetCursor(_defaultCursor);
                SDL_PumpEvents(); // talk to WM
            }
        }};
    }

    Widget(Widget&) = delete;
    
    bool _enabled = true;
    Position _pos;
    Size _size;
    Size _maxSize = Size::UNDEFINED;
    Size _minSize = {0,0};
    Size _autoSize = Size::UNDEFINED;
    int _hGrow = 0;
    int _vGrow = 0;
    Color _backgroundColor;
    bool _visible = true;
    SDL_Cursor *_cursor = nullptr;
    static SDL_Cursor *_defaultCursor;
    Spacing _margin = {0,0,0,0};
    bool _dropShadow = false;
    bool _mouseInteraction = true;

public:
    virtual ~Widget()
    {
        onMouseEnter-=this;
        onMouseLeave-=this;
        onDestroy-=this;
        onDestroy.emit(this);
        if (_cursor) {
            if (SDL_GetCursor() == _cursor) {
                SDL_SetCursor(_defaultCursor);
            }
            SDL_FreeCursor(_cursor);
            _cursor = nullptr;
        }
    }
    
    virtual void render(Renderer renderer, int offX, int offY)=0;
    bool getEnabled() const { return _enabled; }
    virtual void setEnabled(bool enabled) { _enabled = enabled; }
    
    virtual const Position& getPosition() const { return _pos; }
    virtual int getLeft() const { return _pos.left; }
    virtual int getTop() const { return _pos.top; }
    const Size& getSize() const { return _size; }
    int getWidth() const { return _size.width; }
    int getHeight() const { return _size.height; }
    const Size& getAutoSize() const { return _autoSize; }
    int getAutoWidth() const { return _autoSize.width; }
    int getAutoHeight() const { return _autoSize.height; }
    int getHGrow() const { return _hGrow; }
    int getVGrow() const { return _vGrow; }
    void setLeft(int x) { setPosition({x,_pos.top}); }
    void setTop(int y) { setPosition({_pos.left,y}); }
    virtual void setPosition(const Position& pos) { _pos = pos; }
    void setWidth(int w) { setSize({w,_size.height}); }
    void setHeight(int h) { setSize({_size.width,h}); }
    virtual void setSize(Size size) { _size = size; }
    virtual void setGrow(int h, int v) { _hGrow=h; _vGrow=v; }
    virtual void setBackground(Color color) { _backgroundColor = color; }
    virtual int getMinX() const { return _pos.left; }
    virtual int getMinY() const { return _pos.top; }
    virtual int getMaxX() const { return _pos.left + _size.width - 1; }
    virtual int getMaxY() const { return _pos.top + _size.height - 1; }

    virtual bool isHit(int x, int y) const
    {
        return _mouseInteraction && x>=getMinX() && x<=getMaxX() && y>=getMinY() && y<=getMaxY();
    }

    virtual const Widget* getHit(int x, int y) const
    {
        return isHit(x + _pos.left, y + _pos.top) ? this : nullptr;
    }

    const Size& getMinSize() const { return _minSize; }
    int getMinWidth() const { return _minSize.width; }
    int getMinHeight() const { return _minSize.height; }
    const Size& getMaxSize() const { return _maxSize; }
    int getMaxWidth() const { return _maxSize.width; }
    int getMaxHeight() const { return _maxSize.height; }
    
    virtual void setMinSize(Size size) { _minSize = size; }
    virtual void setMaxSize(Size size) { _maxSize = size; }
    
    void setVisible(bool visible) { _visible = visible; }
    bool getVisible() const { return _visible; }
    bool hasMouseInteraction() const { return _mouseInteraction; }

    void setDropShaodw(bool dropShadow) { _dropShadow = dropShadow; }
    bool getDropShadow() const { return _dropShadow; }

    virtual bool isHover(Widget* w) const { return (w == this); }

    void setCursor(Cursor cur) {
        bool isDisplayed = false;
        if (!_defaultCursor) {
            _defaultCursor = SDL_CreateSystemCursor((SDL_SystemCursor)Cursor::DEFAULT);
            // TODO: atexit: SDL_FreeCursor(_defaultCursor);
        }
        if (_cursor) {
            if (SDL_GetCursor() == _cursor) {
                isDisplayed = true;
                SDL_SetCursor(_defaultCursor);
            }
            SDL_FreeCursor(_cursor);
            _cursor = nullptr;
        }
        if (cur == Cursor::DEFAULT || cur == Cursor::CUSTOM) {
            return; // custom not imeplemented
        }
        _cursor = SDL_CreateSystemCursor((SDL_SystemCursor)cur);
        if (isDisplayed) SDL_SetCursor(_cursor);
    }

    void setMargin(const Spacing& margin) { _margin = margin; }
    const Spacing& getMargin() const { return _margin; }

    Signal<int,int,int> onClick; // TODO: MouseClickEventArgs& ?
    Signal<int,int,unsigned> onMouseMove; // TODO: MouseMoveEventArgs& ?
    Signal<int,int,unsigned> onMouseEnter;
    Signal<> onMouseLeave;
    Signal<int,int,unsigned> onScroll;
    Signal<> onDestroy;

#ifndef NDEBUG
    void dumpInfo() {    
        printf("%s [w=%d,h=%d,minw=%d,minh=%d,maxw=%d,maxh=%d,bg=#%02x%02x%02x(%02x),x=%d,y=%d,grow=%d,%d]\n",
            typeid(*this).name(),
            getWidth(), getHeight(), getMinWidth(), getMinHeight(), getMaxWidth(), getMaxHeight(),
            _backgroundColor.r, _backgroundColor.g, _backgroundColor.b, _backgroundColor.a,
            getLeft(), getTop(), getHGrow(), getVGrow());
    }
#endif
};

} // namespace Ui

#endif // _UILIB_WIDGET_H
