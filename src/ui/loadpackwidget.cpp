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
    
    onClick += {this, [this](void *s, int x, int y, int button) {
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
    _curPackLabel = nullptr;
    _curVariantLabel = nullptr;
    for (auto& pack : availablePacks) {
        auto lbl = new Label(0, 0, 0, 0, _font, " " + pack.packName + " " + pack.version); // TODO: button instead of label
        lbl->setGrow(1,0);
        lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
        lbl->setMinSize({64,lbl->getAutoHeight()});
        lbl->setSize({_size.width/2,32}); // TODO; hbox with even split instead
        lbl->setBackground({32,32,32});
        _packs->addChild(lbl);
        lbl->onMouseEnter += {this,[this,pack](void* s, int x, int y, unsigned buttons) {
            if (_curPackLabel == s) return;
            if (_curPackLabel) _curPackLabel->setBackground({32,32,32});
            _curPackLabel = (Label*)s;
            _curPackLabel->setBackground({64,64,64});
            _variants->clearChildren();
            for (auto& variant: pack.variants) {
                auto lbl = new Label(0,0,0,0, _font, " " + variant.name); // TODO: button instead of label
                lbl->setGrow(1,0);
                lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
                lbl->setMinSize(lbl->getAutoSize());
                lbl->setSize({_variants->getWidth(),32});
                lbl->setBackground({64,64,64});
                _variants->addChild(lbl);
                lbl->onMouseLeave += {this,[this,variant](void *s) {
                    if (_curVariantLabel != s) return;
                    _curVariantLabel->setBackground({64,64,64});
                    _curVariantLabel = nullptr;
                }};
                std::string path = pack.path;
                lbl->onMouseEnter += {this,[this](void *s, int x, int y, unsigned buttons) {
                    if (!s || s==_curVariantLabel) return;
                    if (_curVariantLabel) _curVariantLabel->onMouseLeave.emit(_curVariantLabel);
                    _curVariantLabel = (Label*)s;
                    _curVariantLabel->setBackground({96,96,96});
                }};
                lbl->onClick += {this,[this,path,variant](void *s, int x, int y, int button) {
                    if (button == MouseButton::BUTTON_LEFT) {
                        onPackSelected.emit(this,path,variant.variant);
                    }
                }};
            }
            auto spacer = new Label(0, 0, 0, 0, nullptr, "");
            spacer->setGrow(1,1);
            _variants->addChild(spacer);
            _variants->relayout();
            _main->relayout();
        }};
    }
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
