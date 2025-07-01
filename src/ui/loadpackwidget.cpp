#include "loadpackwidget.h"
#include "../core/pack.h"
#include "../uilib/hbox.h"
#include "../uilib/vbox.h"
#include "../uilib/scrollvbox.h"
#include "../uilib/label.h"
#include "defaults.h" // DEFAULT_FONT_*

namespace Ui {

LoadPackWidget::LoadPackWidget(int x, int y, int w, int h, FontStore *fontStore)
        : SimpleContainer(x,y,w,h), _fontStore(fontStore)
{
    _font = _fontStore->getFont(DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE);
    _smallFont = _fontStore->getFont(DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE - 2);
    if (_font && !_smallFont) _smallFont = _font;
    
    onClick += {this, [this](void*, int, int, int button) {
        if (button == MouseButton::BUTTON_RIGHT) {
            setVisible(false); // TODO: fire onAbort ?
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
    addChild(hbox);
    _main = hbox;
}

void LoadPackWidget::update()
{
    auto availablePacks = Pack::ListAvailable();
    _packs->clearChildren();
    _variants->clearChildren();
    _curPackHover = nullptr;
    _curPackLabel = nullptr;
    _curVariantLabel = nullptr;
    _disableHoverSelect = false;

    for (auto& pack : availablePacks) {
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

    _packs->onMouseLeave += {this, [this](void*) {
        if (_disableHoverSelect && _curPackHover) {
            if (_curPackHover == _curPackLabel) {
                _curPackHover->setBackground(PACK_BG_ACTIVE);
            } else {
                _curPackHover->setBackground(PACK_BG_DEFAULT);
            }
        }
    }};

    if (availablePacks.empty()) {
        auto lbl = new Label(0, 0, 0, 0, _font, "No packs installed!");
        lbl->setGrow(1,1);
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
        lbl->setMinSize(lbl->getAutoSize());
        lbl->setSize({_size.width/2,32}); // TODO; hbox with even split instead
        _packs->addChild(lbl);
        auto spacer = new Label(0, 0, 0, 0, nullptr, "");
        spacer->setGrow(1,1);
        _packs->addChild(spacer);
    }
    _main->setSize(_size);
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
