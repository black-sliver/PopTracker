#include "tooltip.h"
#include "container.h"
#include "size.h"
#include "label.h"
#include <string>

namespace Ui {
    
Tooltip::Tooltip()
    : HBox(0, 0, 2*PADDING, 2*PADDING)
{
    _mouseInteraction = false;
    setBackground({0x00, 0x00, 0x00, 0xbf});
    setGrow(1, 1);
    setPadding(PADDING);
}

Tooltip::Tooltip(FONT font, const std::string& text)
    : Tooltip()
{
    Label* label = new Label(0, 0, 0, 0, font, text);
    label->setSize(label->getAutoSize());
    addChild(label);
}

} // namespace Ui
