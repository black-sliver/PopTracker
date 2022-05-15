#ifndef _UILIB_PROGRESSBAR_H
#define _UILIB_PROGRESSBAR_H

#include "widget.h"
#include <string>

namespace Ui {

class ProgressBar : public Widget {
public:
    ProgressBar(int x, int y, int w, int h, int max=0, int progress=0);
    virtual ~ProgressBar();

    virtual void render(Renderer renderer, int offX, int offY);

protected:
    int _max;
    int _progress;

public:
    virtual void setMax(int max);
    virtual int getMax() const { return _max; }
    virtual void setProgress(int progress);
    virtual int getProgress() const { return _progress; }
};

} // namespace Ui

#endif /* _UILIB_PROGRESSBAR_H */
