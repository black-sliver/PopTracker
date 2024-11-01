#ifndef _UILIB_IMAGEBUTTON_H
#define _UILIB_IMAGEBUTTON_H

#include "image.h"
#include "../core/fs.h"

namespace Ui {

class ImageButton : public Image {
public:
    ImageButton(int x, int y, int w, int h, const fs::path& path);
    ImageButton(int x, int y, int w, int h, const void* data, size_t len);
};

} // namespace Ui

#endif // _UILIB_IMAGEBUTTON_H
