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

TEST(GHAPIReleaseAsset, updatedAtGood) {
    // 2025-06-21T09:24:27Z -> 1750497867
    const auto j = nlohmann::json::parse(releasesData);
    gh::api::ReleaseAsset a;
    gh::api::from_json(j[0]["assets"][0], a);
    EXPECT_EQ(a.updatedAt(), 1750497867u);
}

TEST(GHAPIReleaseAsset, updatedAtBad) {
    // 1234 -> exception
    auto j = nlohmann::json::parse(releasesData);
    j[0]["assets"][0]["updated_at"] = "1234";
    gh::api::ReleaseAsset a;
    EXPECT_THROW({
        gh::api::from_json(j[0]["assets"][0], a);
    }, std::exception);
}
