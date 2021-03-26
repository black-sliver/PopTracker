#ifndef _UILIB_GROUP_H
#define _UILIB_GROUP_H

#include "vbox.h"
#include "label.h"

namespace Ui {

class Group : public VBox {
public:
    using FONT = Label::FONT;
    Group(int x, int y, int w, int h, FONT font, const std::string& title);
    virtual void render(Renderer renderer, int offX, int offY) override;

protected:
    Label* _lbl;
};

} // namespace Ui

#endif // _UILIB_GROUP_H
