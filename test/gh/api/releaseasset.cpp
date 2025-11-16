#include "../../../src/gh/api/releaseasset.hpp"
#include <gtest/gtest.h>
#include "releases_data.h"


TEST(GHAPIReleaseAsset, endsWithCI) {
    const auto j = nlohmann::json::parse(releasesData);
    gh::api::ReleaseAsset a0;
    gh::api::ReleaseAsset a1;
    gh::api::from_json(j[0]["assets"][0], a0);
    gh::api::from_json(j[0]["assets"][1], a1);
    ASSERT_EQ(a0.browserDownloadUrl(), "https://github.com/black-sliver/PopTracker/releases/download/v0.32.1/PopTracker-0.32.1-x86_64.AppImage");
    ASSERT_EQ(a1.browserDownloadUrl(), "https://github.com/black-sliver/PopTracker/releases/download/v0.32.1/PopTracker-0.32.1-x86_64.AppImage.minisig");
    EXPECT_FALSE(a0.nameEndsWithCI(".minisig"));
    EXPECT_TRUE(a1.nameEndsWithCI(".minisig"));
    EXPECT_TRUE(a1.nameEndsWithCI(".MiniSig"));
    EXPECT_FALSE(a1.nameEndsWithCI(".AppImage"));
}
