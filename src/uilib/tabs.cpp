#include "tabs.h"

namespace Ui {

// TODO: move to utility file
#ifndef unlikely
#define unlikely(x) __builtin_expect((x),0)
#endif

template<class T>
static T* getFromList(std::list<T*> list, int index)
{
    if (unlikely(list.empty())) return nullptr;
    if (index==0) { // 0 index = first
        return list.front();
    } else if (index<0) { // negative index = Nth from back
        for (auto it = list.rbegin(); it != list.rend(); it++) {
            index++;
            if (index==0) {
                return *it;
            }
        }
    } else { // positive index = Nth from front
        for (auto it = list.begin(); it != list.end(); it++) {
            if (index==0) {
                return *it;
            }
            index--;
        }
    }
    return nullptr;
}

Tabs::Tabs(int x, int y, int w, int h, FONT font)
    : Container(x,y,w,h), _font(font)
{
    _buttonbox = new HFlexBox(0, 0, w, 32, HFlexBox::HAlign::CENTER);
    // hbox lives outside of _children, so we have to handle events manually.
    // TODO: solve this differently? 
    onClick += { this, [this](void*, int x, int y, int button) {
        auto w = _buttonbox;
        if (w->getVisible() &&
                x >= w->getLeft() && x < w->getLeft()+w->getWidth() &&
                y >= w->getTop()  && y < w->getTop() +w->getHeight())
            w->onClick.emit(w, x-w->getLeft(), y-w->getTop(), button);
        
# if 0
        x -= (_size.width-_buttonbox->getWidth())/2;
        for (auto btn : _buttons) {
            if (btn->getVisible() &&
                    x >= btn->getLeft() && x < btn->getLeft()+btn->getWidth() &&
                    y >= btn->getTop() && y < btn->getTop()+btn->getHeight())
                btn->onClick.emit(btn, x-btn->getLeft(), y-btn->getTop(), button);
        }
#endif
    }};
}
Tabs::~Tabs()
{
    delete _buttonbox;
    _buttonbox = nullptr;
}
void Tabs::addChild(Widget* w)
{
    w->setVisible(false);
    _children.push_back(w);
    Button* btn = new Button(0,0,0,0,_font,"Tab");
    btn->setSize(btn->getMinSize());
    btn->onClick += {this, [this](void* sender,int x, int y, int btn) {
        if ((void*)_tabButton == sender) return; // already selected
        int n=0;
        if (_tabButton) _tabButton->setState(Button::State::AUTO);
        if (_tab) _tab->setVisible(false);
        auto childIt = _children.begin();
        for (auto btn: _buttons) {
            if ((void*)btn == sender) {
                _tab = (*childIt);
                _tabIndex = n;
                _tabButton = btn;
                btn->setState(Button::State::PRESSED);
                _tab->setVisible(true);
                break;
            }
            childIt++;
            n++;
        }
    }};
    _buttons.push_back(btn);
    _buttonbox->addChild(btn);
    relayout();
    if (_tab == nullptr) {
        _tab = w;
        _tabIndex = 0;
        _tabButton = btn;
        btn->setState(Button::State::PRESSED);
        _tab->setVisible(true);
    }
    // TODO: fire minsize changed signal?
}
void Tabs::removeChild(Widget *w)
{
    // remove tab and button
    {
        auto childIt = _children.begin();
        auto buttonIt = _buttons.begin();
        for (;childIt!=_children.end(); childIt++,buttonIt++) {
            if (*childIt == w) {
                _children.erase(childIt);
                if (buttonIt != _buttons.end()) {
                    _buttonbox->removeChild(*buttonIt);
                    delete (*buttonIt);
                    _buttons.erase(buttonIt);
                    break;
                }
                relayout();
                break;
            }
        }
    }
    // if selected tab was removed, select next (or previous if no next)
    if (_tab == w) {
        _tab = nullptr;
        auto it=_children.begin();
        for (int i=_tabIndex; it!=_children.end() && i>0; it++, i--);
        if (it != _children.end()) {
            _tab = *it;
        }
        else {
            _tabIndex--;
             if (!_children.empty()) _tab = _children.back();
        }
        if (_tab) _tab->setVisible(true);
    }
}
void Tabs::setTabName(int index, const std::string& name)
{
    auto btn = getFromList(_buttons, index);
    if (btn && btn->getText() != name) {
        btn->setText(name);
        btn->setSize(btn->getMinSize());
        relayout();
    }
}
bool Tabs::setActiveTab(const std::string& name)
{
    for (auto btn: _buttons) {
        if (btn->getText() == name) {
            if (btn != _tabButton) btn->onClick.emit(btn, 0,0,MouseButton::BUTTON_LEFT);
            return true;
        }
    }
    return false;
}
bool Tabs::setActiveTab(int index)
{
    auto btn = getFromList(_buttons, index);
    if (btn) {
        if (btn != _tabButton) btn->onClick.emit(btn, 0,0,MouseButton::BUTTON_LEFT);
        return true;
    }
    return false;
}
static const std::string noTabName = "";
const std::string& Tabs::getActiveTabName() const
{
    if (_tabButton) return _tabButton->getText();
    return noTabName;
}
void Tabs::relayout()
{
    _buttonbox->setWidth(getWidth()); // minHeight depends on width -> set first
    _buttonbox->relayout();
    int top = _buttonbox->getHeight() + _spacing;
    auto size = getSize();
    size.height -= top;
    for (auto& child : _children) {
        child->setTop(top);
        child->setSize(size);
    }
    
    _minSize = _buttonbox->getMinSize();
    for (auto& child : _children) {
        if (_buttonbox->getMinHeight() + child->getMinHeight() + _spacing > _minSize.height) {
            _minSize.height = _buttonbox->getMinHeight() + child->getMinHeight() + _spacing;
        }
        if (child->getMinWidth() > _minSize.width) {
            _minSize.width = child->getMinWidth();
        }
    }
    
    if (_size.width < _minSize.width || _size.height < _minSize.height) {
        setSize(_size||_minSize);
        return;
    }
    _buttonbox->setWidth(_size.width); // NOTE: this change requires halign for buttonbox
}
void Tabs::render(Renderer renderer, int offX, int offY)
{
    offX += _pos.left;
    offY += _pos.top;
    _buttonbox->render(renderer, offX, offY);
    _tab->render(renderer, offX, offY);
}
void Tabs::setSize(Size size)
{
    if (size == _size) return;
    if (size.width < _minSize.width) size.width = _minSize.width;
    if (size.height < _minSize.height) size.height = _minSize.height;
    _size = size;
    relayout();
}


} // namespace

