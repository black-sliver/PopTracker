#ifndef _UILIB_WINDOW_H
#define _UILIB_WINDOW_H

#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "container.h"
#include "fontstore.h"
#include "droptype.h"

#define WINDOW_DEFAULT_POSITION {SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED}

namespace Ui {

class Window : public Container {
protected:
    using FONT = FontStore::FONT;
    
    std::string _title;
    SDL_Window *_win = nullptr;
    SDL_Renderer *_ren = nullptr;
    FontStore *_fontStore = nullptr; // TODO; pass as argument to window constructor?
    FONT _font = nullptr;
    
    void clear();
    void present();
    virtual void render(Renderer renderer, int offX, int offY) override;
    
public:
    using ID = uint32_t;
    Window(const char *title, SDL_Surface* icon=nullptr, const Position& pos=WINDOW_DEFAULT_POSITION, const Size& size={0,0});
    virtual ~Window();
    virtual void render();
    ID getID();
    void setIcon(SDL_Surface* icon);
    virtual void resize(Size size);
    void Raise();
    void setTitle(const std::string& title);
    
    virtual const Position& getPosition() const override;
    virtual void setPosition(const Position& pos) override;
    virtual int getLeft() const override { return getPosition().left; }
    virtual int getTop() const override { return getPosition().top; }
    
    virtual void setMinSize(Size size) override;
    
    Position getPlacementPosition() const;
    int getDisplay() const;
    std::string getDisplayName() const;
    Position getPositionOnDisplay() const;
    void grabFocus();

    Signal<int, int, DropType, std::string> onDrop; // x, y, type, data
};

} // namespace Ui

#endif // _UILIB_WINDOW_H
