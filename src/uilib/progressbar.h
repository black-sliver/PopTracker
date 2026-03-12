#pragma once

#include <string>
#include "timer.h"
#include "widget.h"


namespace Ui {

class ProgressBar : public Widget {
public:
    ProgressBar(int x, int y, int w, int h, int max=0, int progress=0);

    void render(Renderer renderer, int offX, int offY) override;

protected:
    int _max;
    int _progress;
    tick_t _firstIndeterminateDraw = 0;

public:
    virtual void setMax(int max);
    virtual int getMax() const { return _max; }
    virtual void setProgress(int progress);
    virtual int getProgress() const { return _progress; }
};

} // namespace Ui
