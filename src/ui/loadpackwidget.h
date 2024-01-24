#ifndef _UI_LOADPACKWIDGET_H
#define _UI_LOADPACKWIDGET_H

#include "../uilib/container.h"
#include "../uilib/simplecontainer.h"
#include "../uilib/hbox.h"
#include "../uilib/vbox.h"
#include "../uilib/scrollvbox.h"
#include "../uilib/label.h"
#include "../uilib/fontstore.h"
#include "../core/signal.h"
#include <SDL2/SDL_ttf.h>

namespace Ui {

class LoadPackWidget : public SimpleContainer {
public:
    using FONT = FontStore::FONT;
    LoadPackWidget(int x, int y, int w, int h, FontStore *fontStore);
    
    void update();
    
    Signal<const std::string&,const std::string&> onPackSelected;
    
    virtual void setSize(Size size) override; // TODO: have more intelligent hbox instead
    
protected:
    FontStore *_fontStore;
    FONT _font;
    FONT _smallFont;
    
    ScrollVBox *_packs;
    VBox *_variants;
    HBox *_main;
    Label *_curPackLabel = nullptr;
    Label *_curVariantLabel = nullptr;
    Label *_curPackHover = nullptr;
    bool _disableHoverSelect = false;

    static constexpr Color PACK_BG_DEFAULT = {32, 32, 32};
    static constexpr Color PACK_BG_ACTIVE = {32, 128, 32};
    static constexpr Color PACK_BG_HOVER = {64, 64, 64};
    static constexpr Color PACK_BG_ACTIVE_HOVER = {64, 160, 64};
    static constexpr Color VARIANT_BG_DEFAULT = {64, 64, 64};
    static constexpr Color VARIANT_BG_HOVER = {96, 96, 96};
};

} // namespace Ui

#endif /* _UI_LOADPACKWIDGET_H */

