#include <cassert>
#include <vector>
#include <gtest/gtest.h>
#include <SDL2/SDL.h>
#include "../../src/core/assets.h"
#include "../../src/core/fileutil.h"
#include "../../src/uilib/imagefilter.h"


using namespace Ui;

#define ALL_KNOWN_FILTERS \
"grey", \
"disable", \
"overlay"

class ImageFilterSuite : public testing::TestWithParam<std::string_view>
{
};

static std::string getSampleOverlayData() {
    static std::string res;
    if (res.empty()) {
        bool ok = readFile(asset("open.png"), res);
        assert(ok);
    }
    return res;
}

static std::string getPaletteData(SDL_Surface* surf) {
    if (surf->format->palette) {
        return {(const char*)surf->format->palette->colors, sizeof(SDL_Color) * surf->format->palette->ncolors};
    }
    return {};
}

TEST_P(ImageFilterSuite, NullInNullOut) {
    SDL_Surface *surf = nullptr;
    ImageFilter filter(std::string{GetParam()}, getSampleOverlayData());
    EXPECT_EQ(surf, filter.apply(surf));
}

TEST_P(ImageFilterSuite, SomethingInOtherOut) {
    auto surf = IMG_Load(asset("closed.png").u8string().c_str());
    ASSERT_TRUE(surf);
    std::string originalData((const char*)surf->pixels, surf->h * surf->pitch);
    std::string originalPalette = getPaletteData(surf);
    ImageFilter filter(std::string{GetParam()}, getSampleOverlayData());
    SDL_Surface *surf2 = filter.apply(surf);
    ASSERT_TRUE(surf2);
    std::string modifiedData((const char*)surf2->pixels, surf2->h * surf2->pitch);
    if (surf2->format->palette) {
        // filter may just change the palette for images with color palette
        std::string modifiedPalette = getPaletteData(surf2);
        EXPECT_TRUE(originalData != modifiedData || originalPalette != modifiedPalette);
    } else {
        // filter has to modify data if the image has no color palette
        EXPECT_NE(originalData, modifiedData);
    }
    SDL_FreeSurface(surf2);
}

INSTANTIATE_TEST_SUITE_P(
    KnownFilters,
    ImageFilterSuite,
    testing::Values(ALL_KNOWN_FILTERS)
);

TEST(ImageFilterTest, UnknownFilterUnmodied) {
    auto surf = IMG_Load(asset("closed.png").u8string().c_str());
    ASSERT_TRUE(surf);
    std::string originalData((const char*)surf->pixels, surf->h * surf->pitch);
    std::string originalPalette = getPaletteData(surf);
    ImageFilter filter("unknown", getSampleOverlayData());
    SDL_Surface *surf2 = filter.apply(surf);
    ASSERT_TRUE(surf2);
    EXPECT_EQ(surf, surf2);
    std::string modifiedData((const char*)surf2->pixels, surf2->h * surf2->pitch);
    std::string modifiedPalette = getPaletteData(surf2);
    EXPECT_EQ(originalData, modifiedData);
    EXPECT_EQ(originalPalette, modifiedPalette);
    SDL_FreeSurface(surf2);
}

TEST(ImageFilterTest, MissingOverlayUnmodied) {
    auto surf = IMG_Load(asset("closed.png").u8string().c_str());
    ASSERT_TRUE(surf);
    std::string originalData((const char*)surf->pixels, surf->h * surf->pitch);
    std::string originalPalette = getPaletteData(surf);
    ImageFilter filter("overlay", "");
    SDL_Surface *surf2 = filter.apply(surf);
    ASSERT_TRUE(surf2);
    EXPECT_EQ(surf, surf2);
    std::string modifiedData((const char*)surf2->pixels, surf2->h * surf2->pitch);
    std::string modifiedPalette = getPaletteData(surf2);
    EXPECT_EQ(originalData, modifiedData);
    EXPECT_EQ(originalPalette, modifiedPalette);
    SDL_FreeSurface(surf2);
}

TEST(ImageFilterTest, FilteredOverlay) {
    std::string data0;
    std::string palette0;
    std::string data1;
    std::string palette1;
    std::string data2;
    std::string palette2;
    {
        auto surf = IMG_Load(asset("closed.png").u8string().c_str());
        ASSERT_TRUE(surf);
        ImageFilter filter("overlay", getSampleOverlayData());
        SDL_Surface *surf2 = filter.apply(surf);
        ASSERT_TRUE(surf2);
        data1 = std::string((const char*)surf2->pixels, surf2->h * surf2->pitch);
        palette1 = getPaletteData(surf2);
        SDL_FreeSurface(surf2);
    }
    {
        auto surf = IMG_Load(asset("closed.png").u8string().c_str());
        ASSERT_TRUE(surf);
        data0 = std::string((const char*)surf->pixels, surf->h * surf->pitch);
        palette0 = getPaletteData(surf);
        std::vector<std::string> args = {getSampleOverlayData(), "grey"};
        ImageFilter filter("overlay", args);
        SDL_Surface *surf2 = filter.apply(surf);
        ASSERT_TRUE(surf2);
        data2 = std::string((const char*)surf2->pixels, surf2->h * surf2->pitch);
        palette2 = getPaletteData(surf2);
        SDL_FreeSurface(surf2);
    }
    ASSERT_TRUE(data0 != data2 || palette0 != palette2);
    ASSERT_TRUE(data1 != data2 || palette1 != palette2);
}

TEST(ImageFilterTest, CompareArgless) {
    EXPECT_EQ(ImageFilter{"test"}, ImageFilter{"test"});
    EXPECT_FALSE(ImageFilter{"test1"} == ImageFilter{"test2"});
}

TEST(ImageFilterTest, CompareSimpleArg) {
    EXPECT_EQ((ImageFilter{"test", "arg"}), (ImageFilter{"test", "arg"}));
    EXPECT_FALSE((ImageFilter{"test", "arg1"} == ImageFilter{"test", "arg2"}));
}

TEST(ImageFilterTest, CompareArgVector) {
    std::vector<std::string> args1 = {"arg1", "arg2"};
    std::vector<std::string> args2 = {"arg1", "arg2"};
    std::vector<std::string> args3 = {"arg1", "arg2", "arg3"};
    ImageFilter filter("test", args1);
    ImageFilter same("test", args2);
    ImageFilter other("test", args3);
    EXPECT_EQ(filter, same);
    EXPECT_FALSE(filter == other);
}
