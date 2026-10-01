// NOTE: for now this only tests public interface code paths does not verify render output

#include <gtest/gtest.h>
#include "font_helper.h"
#include "../../src/uilib/textfield.h"
#include <SDL2/SDL.h>
#include <cstring>
#include <vector>

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

static SDL_Window*& testWindow()
{
    static SDL_Window* win = nullptr;
    return win;
}

static SDL_Renderer* headlessRenderer()
{
    static bool tried = false;
    static SDL_Renderer* ren = nullptr;
    if (tried)
        return ren;
    tried = true;

    SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy");
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return nullptr;
    SDL_Window* win = SDL_CreateWindow("textfield-test", 0, 0, 256, 32, SDL_WINDOW_HIDDEN);
    testWindow() = win;
    if (!win)
        return nullptr;
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    return ren;
}

struct Signature {
    long long r = 0, g = 0, b = 0;
    int lit = 0;
    bool operator==(const Signature& o) const { return r == o.r && g == o.g && b == o.b && lit == o.lit; }
};

static Signature signature(SDL_Renderer* renderer, const Widget::Color& bg)
{
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(renderer, &w, &h);
    Uint32 enumFmt = 0;
    if (SDL_Texture* target = SDL_GetRenderTarget(renderer))
        SDL_QueryTexture(target, &enumFmt, nullptr, nullptr, nullptr);
    if (enumFmt == 0 && testWindow())
        enumFmt = SDL_GetWindowPixelFormat(testWindow());
    SDL_PixelFormat* fmt = SDL_AllocFormat(enumFmt);
    Signature sig;
    if (!fmt)
        return sig;
    const int bpp = fmt->BytesPerPixel;
    std::vector<Uint8> px((size_t)w * h * bpp);
    SDL_RenderReadPixels(renderer, nullptr, (SDL_PixelFormatEnum)enumFmt, px.data(), w * bpp);
    for (int i = 0; i < w * h; i++) {
        Uint32 raw = 0;
        std::memcpy(&raw, &px[(size_t)i * bpp], bpp);
        Uint8 r = 0, g = 0, b = 0, a = 0;
        SDL_GetRGBA(raw, fmt, &r, &g, &b, &a);
        sig.r += r;
        sig.g += g;
        sig.b += b;
        if (r != bg.r || g != bg.g || b != bg.b)
            sig.lit++;
    }
    SDL_FreeFormat(fmt);
    return sig;
}

static void renderOnce(SDL_Renderer* renderer, TextField& tf, const Widget::Color& bg)
{
    SDL_SetRenderDrawColor(renderer, bg.r, bg.g, bg.b, 0);
    SDL_RenderClear(renderer);
    tf.render(renderer, 0, 0);
    SDL_RenderPresent(renderer);
}

static Signature signatureForFreshField(SDL_Renderer* ren, const Widget::Color& bg,
                                       const std::string& text, const Widget::Color* color = nullptr)
{
    TextField fresh(0, 0, 200, 0, getDefaultFont());
    fresh.setBackground(bg);
    if (color)
        fresh.setTextColor(*color);
    fresh.setText(text);
    renderOnce(ren, fresh, bg);
    return signature(ren, bg);
}

TEST(TextFieldCacheRenderTest, DrawsText) {
    SDL_Renderer* ren = headlessRenderer();
    if (!ren) GTEST_SKIP() << "no headless SDL video driver";
    const Widget::Color bg = {0, 0, 0, 0};
    TextField tf(0, 0, 200, 0, getDefaultFont());   // no window -> no caret
    tf.setBackground(bg);
    tf.setText("MMMM");
    renderOnce(ren, tf, bg);
    EXPECT_GT(signature(ren, bg).lit, 0) << "text drew no pixels";
}

TEST(TextFieldCacheRenderTest, CacheIsStableAcrossRenders) {
    SDL_Renderer* ren = headlessRenderer();
    if (!ren) GTEST_SKIP() << "no headless SDL video driver";
    const Widget::Color bg = {0, 0, 0, 0};
    TextField tf(0, 0, 200, 0, getDefaultFont());
    tf.setBackground(bg);
    tf.setText("MMMM");
    renderOnce(ren, tf, bg);
    const Signature first = signature(ren, bg);
    for (int i = 0; i < 5; i++)
        renderOnce(ren, tf, bg);
    EXPECT_TRUE(signature(ren, bg) == first) << "cached texture renders inconsistently";
}

TEST(TextFieldCacheRenderTest, TextChangeInvalidatesCache) {
    SDL_Renderer* ren = headlessRenderer();
    if (!ren) GTEST_SKIP() << "no headless SDL video driver";
    const Widget::Color bg = {0, 0, 0, 0};
    TextField tf(0, 0, 200, 0, getDefaultFont());
    tf.setBackground(bg);

    tf.setText("MMMM");
    renderOnce(ren, tf, bg);
    const Signature first = signature(ren, bg);
    const Signature expectedSecond = signatureForFreshField(ren, bg, "iiii");
    ASSERT_GT(first.lit, 0);
    ASSERT_GT(expectedSecond.lit, 0);
    ASSERT_FALSE(first == expectedSecond) << "pick two strings that render differently";

    tf.setText("iiii");
    renderOnce(ren, tf, bg);
    EXPECT_TRUE(signature(ren, bg) == expectedSecond) << "stale texture served after text change";

    tf.setText("MMMM");
    renderOnce(ren, tf, bg);
    EXPECT_TRUE(signature(ren, bg) == first) << "re-render differs after retyping same text";
}

TEST(TextFieldCacheRenderTest, PlaceholderChangesInvalidateCache) {
    SDL_Renderer* ren = headlessRenderer();
    if (!ren) GTEST_SKIP() << "no headless SDL video driver";
    const Widget::Color bg = {0, 0, 0, 0};
    TextField tf(0, 0, 200, 0, getDefaultFont());
    tf.setBackground(bg);
    tf.setPlaceholder("MMMM");
    renderOnce(ren, tf, bg);
    const Signature first = signature(ren, bg);
    ASSERT_GT(first.lit, 0) << "placeholder drew no pixels";

    // same trick: compare against a fresh field showing a different placeholder
    TextField fresh(0, 0, 200, 0, getDefaultFont());
    fresh.setBackground(bg);
    fresh.setPlaceholder("iiii");
    renderOnce(ren, fresh, bg);
    const Signature expectedSecond = signature(ren, bg);
    ASSERT_GT(expectedSecond.lit, 0);
    ASSERT_FALSE(first == expectedSecond) << "pick two placeholders that render differently";

    tf.setPlaceholder("iiii");
    renderOnce(ren, tf, bg);
    EXPECT_TRUE(signature(ren, bg) == expectedSecond) << "stale placeholder texture served";
}

TEST(TextFieldCacheRenderTest, TextColorChangeInvalidatesCache) {
    SDL_Renderer* ren = headlessRenderer();
    if (!ren) GTEST_SKIP() << "no headless SDL video driver";
    const Widget::Color bg = {0, 0, 0, 0};
    const Widget::Color red = {255, 0, 0, 255};
    TextField tf(0, 0, 200, 0, getDefaultFont());
    tf.setBackground(bg);
    tf.setText("MMMM");
    renderOnce(ren, tf, bg);
    const Signature white = signature(ren, bg);
    ASSERT_GT(white.lit, 0);
    const Signature expectedRed = signatureForFreshField(ren, bg, "MMMM", &red);
    ASSERT_GT(expectedRed.lit, 0);
    ASSERT_FALSE(white == expectedRed) << "pick a colour that renders differently";

    tf.setTextColor(red);
    renderOnce(ren, tf, bg);
    EXPECT_TRUE(signature(ren, bg) == expectedRed) << "stale texture served after colour change";
    tf.setTextColor({255, 255, 255, 0});
    renderOnce(ren, tf, bg);
    EXPECT_TRUE(signature(ren, bg) == white) << "stale texture served after colour change back";
}

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
