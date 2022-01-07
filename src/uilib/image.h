#ifndef _UILIB_IMAGE_H
#define _UILIB_IMAGE_H

#include "widget.h"
#include <string>

namespace Ui {

class Image : public Widget
{
public:
    Image(int x, int y, int w, int h, const char* path);
    Image(int x, int y, int w, int h, const void* data, size_t len);
    ~Image();
    virtual void render(Renderer renderer, int offX, int offY) override;
    virtual void setSize(Size size) override;
    int getQuality() const { return _quality; }
    // NOTE: this has to be set before the image is rendered for the first time
    virtual void setQuality(int q) { _quality = q; }
    virtual void setDarkenGreyscale(bool value);
protected:
    SDL_Surface *_surf = nullptr;
    SDL_Texture *_tex = nullptr;
    SDL_Texture *_texBw = nullptr;
    std::string _path;
    bool _fixedAspect=true;
    int _quality=-1;
    bool _darkenGreyscale = true; // makes greyscale version look "disabled"
};

} // namespace Ui

#endif // _UILIB_IMAGE_H
