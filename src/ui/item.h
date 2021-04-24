#ifndef _UI_ITEM_H
#define _UI_ITEM_H

#include "../uilib/widget.h"
#include "../uilib/imagefilter.h"
#include <vector>
#include <list>
#include <SDL2/SDL_ttf.h>

namespace Ui {

class Item : public Widget
{
public:
    using FONT = TTF_Font*;
    Item(int x, int y, int w, int h, FONT font);
    ~Item();
    virtual void render(Renderer renderer, int offX, int offY) override;
    virtual void setSize(Size size) override;
    int getQuality() const { return _quality; }
    // NOTE: this has to be set before the image is rendered for the first time
    virtual void setQuality(int q) { _quality = q; }
    
    virtual void setStage(int stage1, int stage2);
    virtual int getStage1() const { return _stage1; }
    virtual int getStage2() const { return _stage2; }
     // FIXME: it would probably make sense to move the image (+modifiers) out of the widget and just apply the final image
    virtual void addStage(int stage1, int stage2, const char *path, std::list<ImageFilter> filters={});
    virtual void addStage(int stage1, int stage2, const void *data, size_t len, std::list<ImageFilter> filters={});
    virtual void setOverlay(const std::string& s);
    virtual void setOverlayColor(Widget::Color color);
    virtual std::string getOverlay() const { return _overlay; }
    virtual Widget::Color getOverlayColor() const { return _overlayColor; }
protected:
    std::vector< std::vector<SDL_Surface*> > _surfs;
    std::vector< std::vector<SDL_Texture*> > _texs; // TODO: use texture store to avoid storing duplicates
    bool _fixedAspect=true;
    int _quality=-1;
    int _stage1=0;
    int _stage2=0;
    virtual void addStage(int stage1, int stage2, SDL_Surface* surf, std::list<ImageFilter> filters={});
    void freeStage(int stage1, int stage2);
    FONT _font;
    std::string _overlay;
    Widget::Color _overlayColor = {255,255,255};
    SDL_Texture *_overlayTex = nullptr;
};

} // namespace Ui

#endif // _UI_ITEM_H
