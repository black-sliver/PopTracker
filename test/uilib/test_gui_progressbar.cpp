#include <chrono>
#include <thread>
#include <gtest/gtest.h>
#include "../../src/uilib/progressbar.h"
#include "../../src/uilib/ui.h"

#define usleep(usec) std::this_thread::sleep_for(std::chrono::microseconds(usec))

TEST(ProgressBarGuiTest, TestGui) {
#ifndef WITH_GUI_TESTS
    GTEST_SKIP();
#endif
    Ui::Ui ui("Test", false);
    Ui::Window* win = ui.createWindow<Ui::Window>("Test", nullptr, Ui::Position::UNDEFINED, {232, 48});
    const auto pgb = new Ui::ProgressBar(16, 16, 200, 6, 100, 50);
    win->addChild(pgb);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 100; k++) {
                ui.render();
                usleep(16666);
                pgb->setProgress(k);
            }
            ui.render();
            usleep(16666);
            if (j == 0)
                pgb->setMax(-1);
            else
                pgb->setMax(100);
        }
        if (i == 0)
            win->setBackground({"#808080"});
        else
            win->setBackground({"#eeeeee"});
    }
    ui.destroyWindow(win);
}
