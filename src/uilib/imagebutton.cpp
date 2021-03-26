#include "imagebutton.h"

namespace Ui {

ImageButton::ImageButton(int x, int y, int w, int h, const char* path)
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

