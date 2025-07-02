// NOTE: for now this only tests public interface code paths does not verify render output

#include <gtest/gtest.h>
#include "font_helper.h"
#include "../../src/uilib/tabs.h"


using namespace Ui;

TEST(TabsTest, PositiveIndex) {
    const auto font = getDefaultFont();
    auto tabs = Tabs(0, 0, 800, 600, font);
    tabs.addChild(new Label(0, 0, 0, 0, font, "One"));
    tabs.addChild(new Label(0, 0, 0, 0, font, "Two"));
    tabs.setTabName(0, "One");
    tabs.setTabName(1, "Two");
    tabs.setActiveTab(0);
    EXPECT_EQ(tabs.getActiveTabName(), "One");
    tabs.setActiveTab(1);
    EXPECT_EQ(tabs.getActiveTabName(), "Two");
    tabs.setActiveTab(2); // invalid
    EXPECT_EQ(tabs.getActiveTabName(), "Two"); // -> unchanged
}

TEST(TabsTest, NegativeIndex) {
    const auto font = getDefaultFont();
    auto tabs = Tabs(0, 0, 800, 600, font);
    tabs.addChild(new Label(0, 0, 0, 0, font, "One"));
    tabs.addChild(new Label(0, 0, 0, 0, font, "Two"));
    tabs.setTabName(-2, "One");
    tabs.setTabName(1, "Two");
    tabs.setActiveTab(0);
    EXPECT_EQ(tabs.getActiveTabName(), "One");
    tabs.setActiveTab(-1);
    EXPECT_EQ(tabs.getActiveTabName(), "Two");
    tabs.setActiveTab(-3); // invalid
    EXPECT_EQ(tabs.getActiveTabName(), "Two"); // -> unchanged
}

TEST(TabsTest, ByName) {
    const auto font = getDefaultFont();
    auto tabs = Tabs(0, 0, 800, 600, font);
    tabs.addChild(new Label(0, 0, 0, 0, font, "One"));
    tabs.addChild(new Label(0, 0, 0, 0, font, "Two"));
    tabs.setTabName(0, "One");
    tabs.setTabName(1, "Two");
    tabs.setActiveTab("One");
    EXPECT_EQ(tabs.getActiveTabName(), "One");
    tabs.setActiveTab("Two");
    EXPECT_EQ(tabs.getActiveTabName(), "Two");
    tabs.setActiveTab("Three"); // invalid
    EXPECT_EQ(tabs.getActiveTabName(), "Two"); // -> unchanged
}

/// Test that reserve only affects capacity and not size and does not hinder growing. Also access some of that memory.
TEST(TabsTest, Reserve) {
    const auto defaultText = "Tab";

    const auto font = getDefaultFont();
    auto defaultTabs = Tabs(0, 0, 800, 600, font);
    auto reservedTabs = Tabs(0, 0, 800, 600, font);
    reservedTabs.reserve(2);

    EXPECT_EQ(defaultTabs.getChildren().size(), 0UL);
    EXPECT_EQ(reservedTabs.getChildren().size(), 0UL);

    for (size_t i = 0; i < 3; ++i) {
        // use reserved memory
        defaultTabs.addChild(new Label(0, 0, 0, 0, font, "Test"));
        reservedTabs.addChild(new Label(0, 0, 0, 0, font, "Test"));

        EXPECT_EQ(defaultTabs.getChildren().size(), i + 1);
        EXPECT_EQ(reservedTabs.getChildren().size(), i + 1);

        // access internal buttons
        defaultTabs.setActiveTab(-1);
        reservedTabs.setActiveTab(-1);
        EXPECT_EQ(defaultTabs.getActiveTabName(), defaultText);
        EXPECT_EQ(reservedTabs.getActiveTabName(), defaultText);
        defaultTabs.setTabName(i, "Test");
        reservedTabs.setTabName(i, "Test");
        EXPECT_EQ(defaultTabs.getActiveTabName(), "Test");
        EXPECT_EQ(reservedTabs.getActiveTabName(), "Test");
        defaultTabs.setTabName(-1, "");
        reservedTabs.setTabName(-1, "");
    }
}
