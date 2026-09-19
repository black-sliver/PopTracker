#include "loadpackwidget.h"
#include "../core/pack.h"
#include "../core/assets.h"
#include "../uilib/hbox.h"
#include "../uilib/vbox.h"
#include "../uilib/scrollvbox.h"
#include "../uilib/label.h"
#include "../uilib/imagebutton.h"
#include "defaults.h" // DEFAULT_FONT_*
#include <SDL2/SDL.h>
#include <cctype>

namespace Ui {

static std::string urlEncode(const std::string& in)
{
    static const char hex[] = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : in) {
        if (isalnum(c) || c=='-' || c=='_' || c=='.' || c=='~' || c=='/')
            out += (char)c;
        else {
            out += '%';
            out += hex[(c>>4)&0xf];
            out += hex[c&0xf];
        }
    }
    return out;
}

static void openPackDirectory()
{
    for (const auto& searchPath : Pack::getSearchPaths()) {
        if (!fs::is_directory(searchPath))
            continue;
        std::string url = "file://" + urlEncode(searchPath.u8string());
        if (SDL_OpenURL(url.c_str()) == 0)
            return;
        fprintf(stderr, "LoadPackWidget: could not open pack directory '%s'\n", searchPath.u8string().c_str());
    }
}

LoadPackWidget::LoadPackWidget(int x, int y, int w, int h, FontStore *fontStore, Window *window)
        : SimpleContainer(x,y,w,h), _fontStore(fontStore), _window(window)
{
    _font = _fontStore->getFont(DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE);
    _smallFont = _fontStore->getFont(DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE - 2);
    if (_font && !_smallFont) _smallFont = _font;
    
    onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_RIGHT) {
            setVisible(false); // TODO: fire onAbort ?
            if (_filter) _filter->releaseFocus();
        }
    }};
    
    auto packs = new ScrollVBox(0,0,0,0);
    packs->setGrow(1,1);
    packs->setPadding(0);
    packs->setSpacing(1);
    _packs = packs;
    
    auto variants = new VBox(0,0,0,0);
    variants->setGrow(1,1);
    variants->setPadding(0);
    variants->setSpacing(1);
    _variants = variants;
    
    auto hbox = new HBox(0,0,0,0);
    hbox->setGrow(1,1);
    hbox->addChild(packs);
    hbox->addChild(variants);
    hbox->setSpacing(1);
    hbox->setPadding(2);
    _main = hbox;
    
    auto filter = new TextField(0,0,0,0, _font, window);
    filter->setGrow(1,0);
    filter->setPlaceholder("Filter packs...");
    filter->setBackground({16,16,16});
    _filter = filter;
    
    auto btnClear = new Button(0,0,0,0, _font, "Clear");
    btnClear->setGrow(0,0);
    btnClear->setMinSize({64, btnClear->getAutoHeight()+4});
    _btnClearFilter = btnClear;
    
    btnClear->onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT) {
            _filter->clear();
        }
    }};
    
    auto btnOpenFolder = new ImageButton(0,0,24,0, asset("closed.png"));
    btnOpenFolder->setGrow(0,0);
    btnOpenFolder->setMinSize({24, 0});
    _btnOpenFolder = btnOpenFolder;
    
    btnOpenFolder->onClick += {this, [](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_LEFT) {
            openPackDirectory();
        }
    }};
    
    filter->onTextChanged += {this, [this](void*, const std::string&) {
        refreshPacks();
    }};
    
    auto filterBar = new HBox(0,0,0,0);
    filterBar->setGrow(1,0);
    filterBar->setPadding(2);
    filterBar->setSpacing(4);
    filterBar->addChild(filter);
    filterBar->addChild(btnClear);
    filterBar->addChild(btnOpenFolder);
    filterBar->setHeight(filter->getHeight());
    _filterBar = filterBar;
    
    auto root = new VBox(0,0,0,0);
    root->setGrow(1,1);
    root->setPadding(0);
    root->setSpacing(1);
    root->addChild(filterBar);
    root->addChild(hbox);
    _root = root;
    addChild(root);
    
    _packs->onMouseLeave += {this, [this](void*) {
        if (_disableHoverSelect && _curPackHover) {
            if (_curPackHover == _curPackLabel) {
                _curPackHover->setBackground(PACK_BG_ACTIVE);
            } else {
                _curPackHover->setBackground(PACK_BG_DEFAULT);
            }
        }
    }};
}

void LoadPackWidget::focusFilter()
{
    if (_filter) _filter->grabFocus();
}

void LoadPackWidget::releaseFilterFocus()
{
    if (_filter) _filter->releaseFocus();
}

void LoadPackWidget::update()
{
    _availablePacks = Pack::ListAvailable();
    if (_filter) _filter->clear();
    refreshPacks();
}

static std::string toLower(const std::string& in)
{
    std::string out = in;
    for (auto& c : out)
        if (c >= 'A' && c <= 'Z') c += 'a'-'A';
    return out;
}

void LoadPackWidget::refreshPacks()
{
    _packs->clearChildren();
    _variants->clearChildren();
    _curPackHover = nullptr;
    _curPackLabel = nullptr;
    _curVariantLabel = nullptr;
    _disableHoverSelect = false;

    std::string filter;
    if (_filter) filter = toLower(_filter->getText());

    int shown = 0;
    for (auto& pack : _availablePacks) {
        if (!filter.empty()) {
            std::string name = toLower(pack.packName + " " + pack.gameName + " " + pack.version);
            if (name.find(filter) == std::string::npos)
                continue;
        }
        shown++;
        auto lbl = new Label(0, 0, 0, 0, _font, " " + pack.packName + " " + pack.version); // TODO: button instead of label
        lbl->setGrow(1,0);
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
        lbl->setMinSize({64,lbl->getAutoHeight()});
        lbl->setSize({_size.width/2,32}); // TODO; hbox with even split instead
        lbl->setBackground(PACK_BG_DEFAULT);
        _packs->addChild(lbl);

        lbl->onMouseEnter += {this,[this,pack](void* s, int, int, unsigned) {
            if (_curPackHover != s) {
                if (_curPackHover == _curPackLabel && _disableHoverSelect)
                    _curPackHover->setBackground(PACK_BG_ACTIVE);
                else if (_curPackHover)
                    _curPackHover->setBackground(PACK_BG_DEFAULT);

                _curPackHover = (Label*)s;
                if (_curPackHover == _curPackLabel && _disableHoverSelect)
                    _curPackHover->setBackground(PACK_BG_ACTIVE_HOVER);
                else
                    _curPackHover->setBackground(PACK_BG_HOVER);
            }
            if (_disableHoverSelect || _curPackLabel == s)
                return;

            if (_curPackLabel)
                _curPackLabel->setBackground(PACK_BG_DEFAULT);

            _curPackLabel = (Label*)s;
            _variants->clearChildren();
            for (auto& variant: pack.variants) {
                auto lbl = new Label(0,0,0,0, _font, " " + variant.name); // TODO: button instead of label
                lbl->setGrow(1,0);
                lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
                lbl->setMinSize({64,lbl->getAutoHeight()});
                lbl->setSize({_variants->getWidth(),32}); // FIXME: this should be done by vbox
                lbl->setBackground(VARIANT_BG_DEFAULT);
                _variants->addChild(lbl);
                lbl->onMouseLeave += {this,[this,variant](void *s) {
                    if (_curVariantLabel != s) return;
                    _curVariantLabel->setBackground(VARIANT_BG_DEFAULT);
                    _curVariantLabel = nullptr;
                }};
                auto path = pack.path;
                lbl->onMouseEnter += {this,[this](void *s, int, int, unsigned) {
                    if (!s || s==_curVariantLabel) return;
                    if (_curVariantLabel) _curVariantLabel->onMouseLeave.emit(_curVariantLabel);
                    _curVariantLabel = (Label*)s;
                    _curVariantLabel->setBackground(VARIANT_BG_HOVER);
                }};
                lbl->onClick += {this,[this,path,variant](void*, int, int, int button) {
                    if (button == MouseButton::BUTTON_LEFT) {
                        onPackSelected.emit(this,path,variant.variant);
                    }
                }};
            }
            auto spacer = new Label(0, 0, 0, 0, nullptr, "");
            spacer->setGrow(1,1);
            _variants->addChild(spacer);
            _main->relayout(); // changes width of _variants
            _variants->relayout(); // changes width of labels
            _main->relayout(); // fix split in hbox // FIXME: this should not be required
        }};

        lbl->onClick += {this, [this](void* s, int x, int y, int buttons) {
            _disableHoverSelect = false;
            ((Label*)s)->onMouseEnter.emit(s, x, y, (unsigned)buttons);
            _disableHoverSelect = true;
            if (_curPackLabel)
                _curPackLabel->setBackground(PACK_BG_ACTIVE_HOVER);
        }};
    }

    if (shown == 0) {
        const char* msg = _availablePacks.empty() ?
            "No packs installed!\nDrag & drop packs into the window to install them." :
            "No packs found!";
        auto* lbl = new Label(0, 0, 0, 0, _font, msg);
        lbl->setGrow(1,1);
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
        lbl->setMinSize(lbl->getAutoSize());
        lbl->setSize({_size.width/2,32}); // TODO; hbox with even split instead
        _packs->addChild(lbl);
        auto* spacer = new Label(0, 0, 0, 0, nullptr, "");
        spacer->setGrow(1,1);
        _packs->addChild(spacer);
    }
    _main->relayout(); // TODO: have this be done automatically
}

void LoadPackWidget::setSize(Size size)
{
    SimpleContainer::setSize(size);
    // TODO: have more intelligent hbox instead
    _packs->setWidth(size.width/2-1);
    _variants->setWidth(size.width/2-1);
    _main->relayout();
}


} // namespace
