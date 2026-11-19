#include "remotelistswidget.h"
#include "../packmanager/packmanager.h"
#include "../uilib/dlg.h"
#include "../core/util.h"
#include "defaults.h" // DEFAULT_FONT_*
#include <algorithm>

namespace Ui {

static std::string trimString(const std::string& s)
{
    const char* ws = " \t\r\n";
    const auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    const auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

RemoteListsWidget::RemoteListsWidget(int x, int y, int w, int h, FontStore *fontStore)
        : SimpleContainer(x, y, w, h), _fontStore(fontStore)
{
    _font = _fontStore->getFont(DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE);
    _smallFont = _fontStore->getFont(DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE - 2);
    if (_font && !_smallFont) _smallFont = _font;

    onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_RIGHT)
            close();
    }};

    auto addText = [this](const std::string& text) {
        auto lbl = new Label(0, 0, 0, 0, _font, text);
        lbl->setGrow(1, 0);
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
        lbl->setMinSize({64, lbl->getAutoHeight()});
        lbl->setSize({0, 28});
        return lbl;
    };

    _lblTitle = addText(" Remote Pack Lists");
    _lblDescription = addText(" You can select or add pack lists to browse and install packs from.");
    _lblNote = addText(" Note: Community pack lists are not maintained by PopTracker.");
    _lblNote->setTextColor({150, 150, 150});

    auto list = new ScrollVBox(0, 0, 0, 0);
    list->setGrow(1, 1);
    list->setPadding(0);
    list->setSpacing(1);
    _list = list;

    auto btnAdd = new Button(0, 0, 0, 0, _font, "Add another from URL");
    btnAdd->setGrow(0, 0);
    btnAdd->setMinSize({180, btnAdd->getAutoHeight() + 4});
    btnAdd->onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT)
            addFromUrl();
    }};

    auto btnDone = new Button(0, 0, 0, 0, _font, "Done");
    btnDone->setGrow(0, 0);
    btnDone->setMinSize({96, btnDone->getAutoHeight() + 4});
    btnDone->onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT)
            close();
    }};

    auto filler = new Label(0, 0, 0, 0, nullptr, "");
    filler->setGrow(1, 0);

    auto buttons = new HBox(0, 0, 0, 0);
    buttons->setGrow(1, 0);
    buttons->setPadding(2);
    buttons->setSpacing(4);
    buttons->addChild(btnAdd);
    buttons->addChild(filler);
    buttons->addChild(btnDone);
    buttons->setHeight(btnAdd->getAutoHeight() + 8);

    auto root = new VBox(0, 0, 0, 0);
    root->setGrow(1, 1);
    root->setPadding(4);
    root->setSpacing(4);
    root->addChild(_lblTitle);
    root->addChild(_lblDescription);
    root->addChild(list);
    root->addChild(buttons);
    root->addChild(_lblNote);
    _root = root;
    addChild(root);
}

void RemoteListsWidget::setEntries(const std::vector<PackListEntry>& entries)
{
    PackListEntry community{PackLists::COMMUNITY_NAME, PackLists::COMMUNITY_URL, false, true};
    std::vector<PackListEntry> custom;
    for (const auto& e : entries) {
        if (e.url == PackLists::COMMUNITY_URL)
            community.enabled = e.enabled;
        else
            custom.push_back(e);
    }
    _entries.clear();
    _entries.push_back(community);
    _entries.insert(_entries.end(), custom.begin(), custom.end());
    rebuild();
}

void RemoteListsWidget::render(Renderer renderer, int offX, int offY)
{
    if (_dirty) {
        _dirty = false;
        rebuild();
    }
    SimpleContainer::render(renderer, offX, offY);
}

void RemoteListsWidget::setSize(Size size)
{
    SimpleContainer::setSize(size);
    _dirty = true;
}

void RemoteListsWidget::changed()
{
    _dirty = true;
    onChanged.emit(this, _entries);
}

void RemoteListsWidget::close()
{
    setVisible(false);
    onClosed.emit(this);
}

void RemoteListsWidget::rebuild()
{
    _list->clearChildren();
    for (size_t i = 0; i < _entries.size(); i++)
        addEntryRows(i);
    auto spacer = new Label(0, 0, 0, 0, nullptr, "");
    spacer->setGrow(1, 1);
    _list->addChild(spacer);

    const int w = _size.width > 16 ? _size.width - 8 : 0;
    _lblTitle->setWidth(w);
    _lblDescription->setWidth(w);
    _lblNote->setWidth(w);
    _root->relayout();
    _root->relayout();
}

void RemoteListsWidget::addEntryRows(size_t i)
{
    const PackListEntry& e = _entries[i];

    const int W = std::max(200, _size.width - 16);
    const int checkW = 40;
    const int btnW = 64;
    const int buttonW = e.builtin ? 0 : 3 * btnW;
    const int textW = std::max(64, W - checkW - buttonW);

    auto row = new HBox(0, 0, 0, 32);
    row->setGrow(1, 0);
    row->setPadding(0);
    row->setSpacing(0);
    row->setHeight(32);

    auto check = new Label(0, 0, 0, 0, _font, e.enabled ? " [x]" : " [ ]");
    check->setGrow(0, 0);
    check->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
    check->setMinSize({checkW, check->getAutoHeight()});
    check->setSize({checkW, 32});
    check->setBackground(ROW_BG);

    std::string title = " " + e.name;
    if (e.builtin)
        title += "  " + std::string(PackLists::COMMUNITY_DESCRIPTION);
    auto text = new Label(0, 0, 0, 0, _font, title);
    text->setGrow(1, 0);
    text->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
    text->setMinSize({64, text->getAutoHeight()});
    text->setSize({textW, 32});
    text->setBackground(ROW_BG);

    row->addChild(check);
    row->addChild(text);

    if (!e.builtin) {
        auto makeButton = [this](const char* caption) {
            auto b = new Button(0, 0, 0, 0, _font, caption);
            b->setGrow(0, 0);
            b->setMinSize({btnW, 32});
            b->setSize({btnW, 32});
            return b;
        };
        auto up = makeButton("Up");
        up->onClick += {this, [this, i](void*, int, int, int button) {
            if (button != MouseButton::BUTTON_LEFT) return;
            if (i > 1 && i < _entries.size()) { // index 0 is the community list and stays on top
                std::swap(_entries[i], _entries[i - 1]);
                changed();
            }
        }};
        auto down = makeButton("Down");
        down->onClick += {this, [this, i](void*, int, int, int button) {
            if (button != MouseButton::BUTTON_LEFT) return;
            if (i >= 1 && i + 1 < _entries.size()) {
                std::swap(_entries[i], _entries[i + 1]);
                changed();
            }
        }};
        auto remove = makeButton("Remove");
        remove->onClick += {this, [this, i](void*, int, int, int button) {
            if (button != MouseButton::BUTTON_LEFT) return;
            if (i < 1 || i >= _entries.size()) return;
            if (Dlg::MsgBox("PopTracker", "Remove pack list \"" + sanitize_print(_entries[i].name) + "\"?",
                    Dlg::Buttons::YesNo, Dlg::Icon::Question) != Dlg::Result::Yes)
                return;
            _entries.erase(_entries.begin() + i);
            changed();
        }};
        row->addChild(up);
        row->addChild(down);
        row->addChild(remove);
    }
    _list->addChild(row);

    const std::string address = e.builtin ? std::string(PackLists::COMMUNITY_HOMEPAGE) : e.url;
    auto url = new Label(0, 0, 0, 0, _smallFont, "      " + address);
    url->setGrow(1, 0);
    url->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
    url->setMinSize({64, url->getAutoHeight()});
    url->setSize({W, 22});
    url->setTextColor({150, 150, 150});
    url->setBackground(ROW_BG);
    _list->addChild(url);

    auto toggle = [this, i, check](void*, int, int, int button) {
        if (button != MouseButton::BUTTON_LEFT || i >= _entries.size()) return;
        _entries[i].enabled = !_entries[i].enabled;
        check->setText(_entries[i].enabled ? " [x]" : " [ ]");
        onChanged.emit(this, _entries);
    };
    check->onClick += {this, toggle};
    text->onClick += {this, toggle};
    url->onClick += {this, toggle};
}

void RemoteListsWidget::addFromUrl()
{
    if (_validating)
        return;

    std::string name;
    if (!Dlg::InputBox("PopTracker", "Enter a name for the pack list", "", name))
        return;
    name = trimString(name);
    if (name.empty()) {
        Dlg::MsgBox("PopTracker", "Please enter a name.", Dlg::Buttons::OK, Dlg::Icon::Error);
        return;
    }

    std::string url;
    if (!Dlg::InputBox("PopTracker", "Enter the address (https://...) of the pack list", "", url))
        return;
    url = trimString(url);
    if (!PackManager::isValidRepositoryUrl(url)) {
        Dlg::MsgBox("PopTracker", "The address has to start with https://", Dlg::Buttons::OK, Dlg::Icon::Error);
        return;
    }
    const bool duplicate = url == PackLists::COMMUNITY_URL ||
        std::any_of(_entries.begin(), _entries.end(), [&url](const PackListEntry& e) { return e.url == url; });
    if (duplicate) {
        Dlg::MsgBox("PopTracker", "This pack list is already in the list.", Dlg::Buttons::OK, Dlg::Icon::Info);
        return;
    }

    auto add = [this, name, url]() {
        PackListEntry e;
        e.name = name;
        e.url = url;
        e.enabled = true;
        _entries.push_back(e);
        changed();
    };
    if (!_packManager) {
        add();
        return;
    }

    _validating = true;
    _packManager->testRepository(url, [this, url, add](bool valid) {
        _validating = false;
        if (!valid) {
            Dlg::MsgBox("PopTracker",
                    "Could not load a valid pack list from:\n" + sanitize_print(url),
                    Dlg::Buttons::OK, Dlg::Icon::Error);
            return;
        }
        add();
    });
}

} // namespace Ui
