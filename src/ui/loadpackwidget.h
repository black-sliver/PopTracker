#ifndef _UI_LOADPACKWIDGET_H
#define _UI_LOADPACKWIDGET_H

#include "../uilib/container.h"
#include "../uilib/simplecontainer.h"
#include "../uilib/vbox.h"
#include "../uilib/hbox.h"
#include "../uilib/label.h"
#include "../core/signal.h"
#include <SDL2/SDL_ttf.h>

namespace Ui {

class LoadPackWidget : public SimpleContainer {
public:
    using FONT = TTF_Font*;
    LoadPackWidget(int x, int y, int w, int h, FONT font, FONT smallFont=nullptr);
    
    void update();
    
    Signal<const std::string&,const std::string&> onPackSelected;
    
    virtual void setSize(Size size) override; // TODO: have more intelligent hbox instead
    
protected:
    FONT _font;
    FONT _smallFont;
    
    VBox *_packs;
    VBox *_variants;
    HBox *_main;
    Label *_curPackLabel = nullptr;
    Label *_curVariantLabel = nullptr;
};

} // namespace Ui

#endif /* _UI_LOADPACKWIDGET_H */

