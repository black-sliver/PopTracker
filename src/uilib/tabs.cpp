#include "tabs.h"

namespace Ui {

// TODO: move to utility file
#ifndef likely
#define likely(x) __builtin_expect((x),1)
#endif

template<class T>
static T* getFromVector(const std::vector<T*>& vec, int index)
{
    if (index >= 0 && likely(static_cast<size_t>(index) < vec.size())) {
        // positive index = Nth from front
        return vec[index];
    }
    if (index < 0 && likely(static_cast<size_t>(std::abs(index)) <= vec.size())) {
        // negative index = Nth from back
        return vec[vec.size() + index];
    }
    return nullptr;
}

Tabs::Tabs(const int x, const int y, const int w, const int h, FONT font)
    : Container(x,y,w,h), _font(font)
{
    _buttonbox = new HFlexBox(0, 0, w, 32, HFlexBox::HAlign::CENTER);
    Container::addChild(_buttonbox);
}

Tabs::~Tabs()
{
    _buttonbox = nullptr;
    _tab = nullptr;
}

void Tabs::addChild(Widget* w)
{
    w->setVisible(false);
    _children.push_back(w);
    if (w->getHGrow() > _hGrow)
        _hGrow = w->getHGrow();
    auto* btn = new Button(0,0,0,0,_font,"Tab");
    btn->setSize(btn->getMinSize());
    btn->onClick += {this, [this](const void* sender, int, int, int) {
        if (static_cast<void *>(_tabButton) == sender)
            return; // already selected
        int n=0;
        if (_tabButton)
            _tabButton->setState(Button::State::AUTO);
        if (_tab)
            _tab->setVisible(false);
        auto childIt = _children.begin();
        ++childIt; // first child is button box -> skip
        for (const auto btn: _buttons) {
            if (static_cast<void *>(btn) == sender) {
                _tab = (*childIt);
                _tabIndex = n;
                _tabButton = btn;
                btn->setState(Button::State::PRESSED);
                _tab->setVisible(true);
                break;
            }
            ++childIt;
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
        ++childIt; // first child is button box -> skip
        auto buttonIt = _buttons.begin();
        for (; childIt!=_children.end(); ++childIt, ++buttonIt) {
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
        auto it = _children.begin();
        ++it; // first child is button box -> skip
        for (int i = _tabIndex; it != _children.end() && i > 0; ++it, --i) {}
        if (it != _children.end()) {
            _tab = *it;
        } else {
            --_tabIndex;
            if (!_children.empty())
                _tab = _children.back();
        }
        if (_tab)
            _tab->setVisible(true);
    }
}

void Tabs::setTabName(const int index, const std::string& name)
{
    const auto btn = getFromVector(_buttons, index);
    if (btn && btn->getText() != name) {
        btn->setText(name);
        btn->setSize(btn->getMinSize());
        relayout();
    }
}

void Tabs::setTabIcon(const int index, const void *data, const size_t len)
{
    if (const auto btn = getFromVector(_buttons, index)) {
        btn->setIcon(data, len);
        btn->setSize(btn->getMinSize());
        relayout();
    }
}

bool Tabs::setActiveTab(const std::string& name)
{
    return std::any_of(_buttons.begin(), _buttons.end(), [&](auto btn) {
        if (btn->getText() == name) {
            if (btn != _tabButton)
                btn->onClick.emit(btn, 0, 0, MouseButton::BUTTON_LEFT);
            return true;
        }
        return false;
    });
}

bool Tabs::setActiveTab(const int index)
{
    if (const auto btn = getFromVector(_buttons, index)) {
        if (btn != _tabButton)
            btn->onClick.emit(btn, 0, 0, MouseButton::BUTTON_LEFT);
        return true;
    }
    return false;
}

static const std::string noTabName;

const std::string& Tabs::getActiveTabName() const
{
    if (_tabButton)
        return _tabButton->getText();
    return noTabName;
}

void Tabs::relayout()
{
    _buttonbox->setWidth(getWidth()); // minHeight depends on width -> set first
    _buttonbox->relayout();
    int top = _buttonbox->getHeight() + _spacing;
    auto size = getSize();
    size.height -= top;
    for (const auto child : _children) {
        if (child == _buttonbox)
            continue;
        child->setTop(top);
        child->setSize(size);
    }

    _minSize = _buttonbox->getMinSize();
    for (const auto child : _children) {
        if (child == _buttonbox)
            continue;
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
    if (_tab)
        _tab->render(renderer, offX, offY);
}

void Tabs::setSize(Size size)
{
    if (size == _size)
        return;
    if (size.width < _minSize.width)
        size.width = _minSize.width;
    if (size.height < _minSize.height)
        size.height = _minSize.height;
    _size = size;
    relayout();
}

void Tabs::reserve(const size_t size)
{
    _buttons.reserve(size);
}

} // namespace
