#include "imagebutton.h"
#include "../core/fs.h"

namespace Ui {

ImageButton::ImageButton(int x, int y, int w, int h, const fs::path& path)
    : Image(x,y,w,h,path)
{
    setCursor(Cursor::HAND);
}

ImageButton::ImageButton(int x, int y, int w, int h, const void* data, size_t len)
    : Image(x,y,w,h,data,len)
{
    setCursor(Cursor::HAND);
}

} // namespace

