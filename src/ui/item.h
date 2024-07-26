#ifndef _UI_ITEM_H
#define _UI_ITEM_H

#include "../uilib/widget.h"
#include "../uilib/imagefilter.h"
#include <vector>
#include <list>
#include <SDL2/SDL_ttf.h>
#include "../uilib/label.h"

namespace Ui {

class Item : public Widget
{
public:
    using FONT = TTF_Font*;
    Item(int x, int y, int w, int h, FONT font);
    ~Item();
    virtual void render(Renderer renderer, int offX, int offY) override;
    virtual void setSize(Size size) override;
    virtual void setFont(FONT font);
    int getQuality() const { return _quality; }
    // NOTE: this has to be set before the image is rendered for the first time
    virtual void setQuality(int q) { _quality = q; }
    
    virtual void setStage(int stage1, int stage2);
    virtual int getStage1() const { return _stage1; }
    virtual int getStage2() const { return _stage2; }
     // FIXME: it would probably make sense to move the image (+modifiers) out of the widget and just apply the final image
    virtual void addStage(int stage1, int stage2, const char *path, std::list<ImageFilter> filters={});
    virtual void addStage(int stage1, int stage2, const void *data, size_t len, const std::string& name,
                          std::list<ImageFilter> filters={});
    virtual bool isStage(int stage1, int stage2, const std::string& name, std::list<ImageFilter> filters);
    virtual void setOverlay(const std::string& s);
    virtual void setOverlayColor(Widget::Color color);
    virtual void setOverlayBackgroundColor(Widget::Color color);
    virtual void setOverlayAlignment(Label::HAlign halign);
    virtual std::string getOverlay() const { return _overlay; }
    virtual Widget::Color getOverlayColor() const { return _overlayColor; }
    virtual int getMinX() const override { return _renderPos.left; }
    virtual int getMinY() const override { return _renderPos.top; }
    virtual int getMaxX() const override { return _renderPos.left + _renderSize.width - 1; }
    virtual int getMaxY() const override { return _renderPos.top + _renderSize.height - 1; }
    void setImageAlignment(Label::HAlign halign, Label::VAlign valign) {
        _halign = halign;
        _valign = valign;
    }
    void setImageOverride(const void *data, size_t len, const std::string& name, const std::list<ImageFilter>& filters);
    void clearImageOverride();

protected:
    std::vector< std::vector<SDL_Surface*> > _surfs; // TODO: put surf, name and filters in a struct
    std::vector< std::vector<SDL_Texture*> > _texs; // TODO: use texture store to avoid storing duplicates
    std::vector< std::vector<std::string> > _names;
    std::vector< std::vector<std::list<ImageFilter>> > _filters;
    bool _fixedAspect=true;
    int _quality=-1;
    int _stage1=0;
    int _stage2=0;
    Size _renderSize;
    Position _renderPos;
    FONT _font;
    std::string _overlay;
    Widget::Color _overlayColor = {255,255,255};
    Widget::Color _overlayBackgroundColor = {};
    Label::HAlign _overlayAlign = Label::HAlign::RIGHT;
    SDL_Texture *_overlayTex = nullptr;
    Label::HAlign _halign = Label::HAlign::LEFT;
    Label::VAlign _valign = Label::VAlign::TOP;
    SDL_Surface *_overrideSurf = nullptr;
    SDL_Texture *_overrideTex = nullptr;
    std::string _overrideName;
    std::list<ImageFilter> _overrideFilters;

    virtual void addStage(int stage1, int stage2, SDL_Surface* surf, const std::string& name,
                          std::list<ImageFilter> filters={});
    void freeStage(int stage1, int stage2);
};

} // namespace Ui

#endif // _UI_ITEM_H
