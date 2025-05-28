#include <string>
#include <gtest/gtest.h>
#include "../../src/uilib/widget.h"


using namespace Ui;
using std::string_literals::operator""s;


TEST(UiWidgetColorTest, FromStringWithDefaultAlpha) {
    Widget::Color c1 = "#ccff0000"s;
    Widget::Color c2 = "#00ff00"s;
    Widget::Color c3 = Widget::Color::FromStringWithDefaultAlpha("#ff0000", 0xcc);
    Widget::Color c4 = Widget::Color::FromStringWithDefaultAlpha("#00ff00", 0xff);
    Widget::Color c5 = Widget::Color::FromStringWithDefaultAlpha("#110000ff", 0xaa);
    ASSERT_EQ(c1.a, 0xcc);
    ASSERT_EQ(c2.a, 0xff);
    ASSERT_EQ(c3.a, 0xcc);
    ASSERT_EQ(c4.a, 0xff);
    ASSERT_EQ(c5.a, 0x11);
}
