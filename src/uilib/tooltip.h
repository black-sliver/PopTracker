#pragma once


#include "container.h"
#include "hbox.h"
#include "size.h"
#include "label.h"
#include "timer.h"
#include "../ui/defaults.h"
#include <string>


namespace Ui {
    
class Tooltip : public HBox {
public:
    using FONT = Label::FONT;

    Tooltip();
    Tooltip(FONT font, const std::string& text);
    
    virtual ~Tooltip() {};

    static constexpr int PADDING=8;
    static constexpr int OFFSET=2;
    static constexpr tick_t delay = DEFAULT_TOOLTIP_DELAY;

    void setText(const std::string& text);
private:

};

} // namespace Ui
