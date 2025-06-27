#include <algorithm>
#include <cassert>
#include <gtest/gtest.h>
#include <SDL2/SDL.h>
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
