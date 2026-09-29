// NOTE: for now this only tests public interface code paths does not verify render output

#include <gtest/gtest.h>
#include "font_helper.h"
#include "../../src/uilib/textfield.h"

using namespace Ui;

static size_t codepointLen(const std::string& s, size_t i)
{
    const unsigned char b = (unsigned char)s[i];
    if (b < 0x80) return 1;
    if ((b & 0xe0) == 0xc0) return 2;
    if ((b & 0xf0) == 0xe0) return 3;
    if ((b & 0xf8) == 0xf0) return 4;
    return 1;
}

static bool isAtMostOneCodepointRemoved(const std::string& orig, const std::string& res)
{
    if (res == orig) return true;
    for (size_t i = 0; i < orig.size(); ) {
        const size_t n = codepointLen(orig, i);
        if (orig.substr(0, i) + orig.substr(i + n) == res) return true;
        i += n;
    }
    return false;
}

static int textWidth(const std::string& s)
{
    int w = 0, h = 0;
    TTF_SizeUTF8(getDefaultFont(), s.c_str(), &w, &h);
    return w;
}

static void key(TextField& tf, int k)
{
    tf.onKeyDown.emit(nullptr, k, 0);
}

static const std::string A        = "a";
static const std::string B        = "b";
static const std::string E_ACUTE  = "\xC3\xA9";                     // e-acute, 2 bytes
static const std::string HI       = "\xE6\x97\xA5";                 // CJK, 3 bytes
static const std::string NIHON    = "\xE6\x97\xA5" "\xE6\x9C\xAC";  // two CJK chars
static const std::string GAME     = "\xF0\x9F\x8E\xAE";             // 4 bytes

TEST(TextFieldTest, AsciiStillEdits) {
    TextField tf(0, 0, 200, 0, getDefaultFont());
    tf.setText("abc");
    key(tf, SDLK_BACKSPACE);
    EXPECT_EQ(tf.getText(), "ab");
    key(tf, SDLK_LEFT);
    key(tf, SDLK_BACKSPACE);
    EXPECT_EQ(tf.getText(), "b");
    key(tf, SDLK_HOME);
    key(tf, SDLK_DELETE);
    EXPECT_EQ(tf.getText(), "");
}

TEST(TextFieldTest, BackspaceRemovesWholeCodepoint) {
    for (const auto& cp : { E_ACUTE, HI, GAME }) {
        TextField tf(0, 0, 200, 0, getDefaultFont());
        tf.setText(A + cp);
        key(tf, SDLK_BACKSPACE);
        EXPECT_EQ(tf.getText(), A) << "left a partial codepoint behind";
    }
}

TEST(TextFieldTest, DeleteRemovesWholeCodepoint) {
    for (const auto& cp : { E_ACUTE, HI, GAME }) {
        TextField tf(0, 0, 200, 0, getDefaultFont());
        tf.setText(cp + A);
        key(tf, SDLK_HOME);
        key(tf, SDLK_DELETE);
        EXPECT_EQ(tf.getText(), A) << "left a partial codepoint behind";
    }
}

TEST(TextFieldTest, LeftArrowSkipsWholeCodepoint) {
    TextField tf(0, 0, 200, 0, getDefaultFont());
    tf.setText(A + E_ACUTE + B);
    key(tf, SDLK_LEFT);
    key(tf, SDLK_BACKSPACE);
    EXPECT_EQ(tf.getText(), "ab");
}

TEST(TextFieldTest, RightArrowSkipsWholeCodepoint) {
    TextField tf(0, 0, 200, 0, getDefaultFont());
    tf.setText(A + E_ACUTE + B);
    key(tf, SDLK_HOME);
    key(tf, SDLK_RIGHT);
    key(tf, SDLK_RIGHT);
    key(tf, SDLK_DELETE);
    EXPECT_EQ(tf.getText(), A + E_ACUTE);
}

TEST(TextFieldTest, HomeAndEndReachEnds) {
    TextField tf(0, 0, 200, 0, getDefaultFont());
    tf.setText(NIHON);
    key(tf, SDLK_HOME);
    key(tf, SDLK_BACKSPACE); // no-op at the start
    EXPECT_EQ(tf.getText(), NIHON);
    key(tf, SDLK_END);
    key(tf, SDLK_BACKSPACE); // removes the final codepoint only
    EXPECT_EQ(tf.getText(), HI);
}

TEST(TextFieldTest, BackspacingToEmptyLeavesNoDebris) {
    for (const auto& text : { A + E_ACUTE + NIHON + GAME, GAME + GAME, NIHON, E_ACUTE }) {
        TextField tf(0, 0, 200, 0, getDefaultFont());
        tf.setText(text);
        for (int guard = 0; !tf.getText().empty() && guard < 64; guard++) {
            key(tf, SDLK_BACKSPACE);
        }
        EXPECT_EQ(tf.getText(), "");
    }
}

TEST(TextFieldTest, ClickNeverLandsInsideCodepoint) {
    const std::string text = A + E_ACUTE + NIHON + GAME;
    const int width = textWidth(text);
    for (int x = 0; x <= width; x++) {
        TextField tf(0, 0, 200, 0, getDefaultFont());
        tf.setText(text);
        tf.onClick.emit(nullptr, x, 0, BUTTON_LEFT);
        key(tf, SDLK_BACKSPACE);
        EXPECT_TRUE(isAtMostOneCodepointRemoved(text, tf.getText()))
            << "click at x=" << x << " corrupted the text";
    }
}

TEST(TextFieldTest, EmptyFieldIsSafe) {
    TextField tf(0, 0, 200, 0, getDefaultFont());
    for (int k : { SDLK_BACKSPACE, SDLK_DELETE, SDLK_LEFT, SDLK_RIGHT }) {
        key(tf, k);
    }
    EXPECT_EQ(tf.getText(), "");
    tf.onClick.emit(nullptr, 5, 0, BUTTON_LEFT);
    key(tf, SDLK_BACKSPACE);
    EXPECT_EQ(tf.getText(), "");
}
