#include <algorithm>
#include <cassert>
#include <gtest/gtest.h>
#include <SDL2/SDL.h>
#include "../../src/core/fileutil.h"
#include "../../src/core/fs.h"
#include "../../src/uilib/imghelper.h"


using namespace Ui;


static std::vector<fs::path> files(const std::string& subFolder)
{
    std::vector<fs::path> files;
    const fs::path folder = fs::u8path("test/uilib/data/img/" + subFolder);
    for (const auto& dirEntry: fs::recursive_directory_iterator{folder}) {
        if ((dirEntry.is_regular_file() || dirEntry.is_symlink()))
            files.emplace_back(dirEntry.path());
    }
    assert(files.size() > 0);
    return files;
}

class GoodImageFileSuite : public testing::TestWithParam<fs::path> {
};

TEST_P(GoodImageFileSuite, LoadImage) {
    const fs::path& f = GetParam();
    EXPECT_TRUE(fs::exists(f));
    SDL_Surface *surf = LoadImage(f);
    ASSERT_TRUE(surf);
    SDL_FreeSurface(surf);
}

TEST_P(GoodImageFileSuite, DetectImageFormat) {
    // tests detecting the image format from signature
    const fs::path& f = GetParam();
    constexpr auto len = getMaxImageSignatureLength();
    std::string buf;
    ASSERT_TRUE(readFile(f.u8string(), buf, len));
    const auto fmt = detectImageFormat(buf);
    const auto ext = f.extension().u8string();
    if (ext == ".bmp") {
        EXPECT_EQ(ImageFormat::BMP, fmt);
    } else if (ext == ".png") {
        EXPECT_EQ(ImageFormat::PNG, fmt);
    } else if (ext == ".gif") {
        EXPECT_EQ(ImageFormat::GIF, fmt);
    } else if (ext == ".jpg") {
        EXPECT_EQ(ImageFormat::JPEG, fmt);
    } else {
        FAIL() << "Unsupported image format file extension: " << ext;
    }
}

TEST_P(GoodImageFileSuite, GetImageSize) {
    // tests getting image size from just the header for lazy loading
    const fs::path& f = GetParam();
    constexpr auto len = getMaxImageHeaderLength();
    std::string buf;
    ASSERT_TRUE(readFile(f.u8string(), buf, len));
    const auto sz = getImageSize(buf);
    if (detectImageFormat(buf) == ImageFormat::GIF) {
        // getGIFSize() is incompatible to LoadImage(), see getImageSize() for details.
        EXPECT_EQ(Size::UNDEFINED, sz);
        return;
    }
    const auto name = f.filename().u8string();
    if (name.rfind("2x1", 0) == 0) {
        EXPECT_EQ(sz.width, 2) << "expected w=2";
        EXPECT_EQ(sz.height, 1) << "expected h=1";
    } else {
        EXPECT_EQ(sz.width, 1) << "expected w=1";
        EXPECT_EQ(sz.height, 1) << "expected h=1";
    }
    // compare to the size claimed by LoadImage
    SDL_Surface *surf = LoadImage(f);
    ASSERT_TRUE(surf);
    EXPECT_EQ(surf->w, static_cast<int>(sz.width));
    EXPECT_EQ(surf->h, static_cast<int>(sz.height));
    SDL_FreeSurface(surf);
}

class BadImageFileSuite : public testing::TestWithParam<fs::path> {
};

TEST_P(BadImageFileSuite, LoadImage) {
    const fs::path& f = GetParam();
    EXPECT_TRUE(fs::exists(f));
    SDL_Surface *surf = LoadImage(f);
    EXPECT_FALSE(surf);
    SDL_FreeSurface(surf);
}

auto paramNaming = [](const testing::TestParamInfo<GoodImageFileSuite::ParamType>& info) {
    std::string name = info.param.filename().u8string();
    std::replace(name.begin(), name.end(), '.', '_');
    std::replace(name.begin(), name.end(), '-', '_');
    return name;
};

INSTANTIATE_TEST_SUITE_P(
    GoodImageFiles,
    GoodImageFileSuite,
    testing::ValuesIn(files("good")),
    paramNaming
);

INSTANTIATE_TEST_SUITE_P(
    BadImageFiles,
    BadImageFileSuite,
    testing::ValuesIn(files("bad")),
    paramNaming
);
