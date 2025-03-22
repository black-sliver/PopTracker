#include <gtest/gtest.h>
#include "../../src/uilib/fontstore.h"
#include "../../src/uilib/label.h"
#include "../../src/uilib/textutil.h"
#include "../../src/ui/defaults.h"
#include "../../src/core/assets.h"


using namespace Ui;

static FontStore fontStore;

static FontStore::FONT getDefaultFont() {
    static FontStore::FONT res = nullptr;
    if (!res) {
        Assets::addSearchPath("assets");
        TTF_Init();
        res = fontStore.getFont(DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE);
    }
    return res;
}

TEST(UiTextUtilTest, RenderBasicText) {
    auto font = getDefaultFont();
    ASSERT_TRUE(font);
    auto surf = RenderText(font, "Hello World!", {0, 0, 0, 255}, Label::HAlign::CENTER);
    EXPECT_TRUE(surf);
    SDL_FreeSurface(surf);
}

TEST(UiTextUtilTest, SizeTextHorizontal) {
    auto font = getDefaultFont();
    ASSERT_TRUE(font);
    int w1, w2, h1, h2;
    ASSERT_EQ(SizeText(font, "A", &w1, &h1), 0); // non-zero means error
    ASSERT_GE(w1, 1);
    ASSERT_GE(h1, 1);
    ASSERT_EQ(SizeText(font, "AA", &w2, &h2), 0); // non-zero means error
    ASSERT_EQ(h2, h1);
    ASSERT_NEAR(w2, 2 * w1, 1);
}

TEST(UiTextUtilTest, SizeTextVertical) {
    auto font = getDefaultFont();
    ASSERT_TRUE(font);
    int w1, w2, h1, h2;
    ASSERT_EQ(SizeText(font, "A", &w1, &h1), 0); // non-zero means error
    ASSERT_GE(w1, 1);
    ASSERT_GE(h1, 1);
    ASSERT_EQ(SizeText(font, "A\nA", &w2, &h2), 0); // non-zero means error
    ASSERT_EQ(w1, w2);
    ASSERT_GE(h2, 2 * h1);
}

TEST(UiTextUtilTest, BreakText) {
    auto font = getDefaultFont();
    ASSERT_TRUE(font);
    EXPECT_EQ(BreakText(font, "Hello World!", 0), "Hello World!") << "width = 0 should never split";
    EXPECT_EQ(BreakText(font, "Hello World!", 1), "Hello\nWorld!") << "width = 1 should always split";
    EXPECT_EQ(BreakText(font, "Hello\nline break!", 1), "Hello\nline\nbreak!") << "width = 1 should always split";
    EXPECT_EQ(BreakText(font, "Hello World!", 1000), "Hello World!") << "width = 1000 should fit both words";
    EXPECT_EQ(BreakText(font, "Hello\nline break!", 1000), "Hello\nline break!") << "width = 1000 should keep breaks";
    auto broken = BreakText(font, "A A A A A A A A A A A A A A", 50);
    EXPECT_NE(broken.find(" "), std::string::npos) << "width = 50 should fit more than 1 word";
    EXPECT_NE(broken.find("\n"), std::string::npos) << "width = 50 should not fit all words";
}
