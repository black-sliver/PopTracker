#pragma once

#include "../../src/core/assets.h"
#include "../../src/uilib/fontstore.h"
#include "../../src/ui/defaults.h"


static Ui::FontStore fontStore;

static Ui::FontStore::FONT getDefaultFont() {
    static Ui::FontStore::FONT res = nullptr;
    if (!res) {
        Assets::addSearchPath("assets");
        TTF_Init();
        res = fontStore.getFont(Ui::DEFAULT_FONT_NAME, Ui::DEFAULT_FONT_SIZE);
    }
    return res;
}
