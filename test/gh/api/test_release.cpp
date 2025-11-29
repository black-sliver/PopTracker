#include "../../../src/gh/api/release.hpp"
#include <gtest/gtest.h>
#include "releases_data.h"
#include "../../../src/core/fs.h"
#include "../../../src/core/fileutil.h"


TEST(GHAPIRelease, tagName) {
    const auto j = nlohmann::json::parse(releasesData)[0];
    gh::api::Release r;
    gh::api::from_json(j, r);
    ASSERT_EQ(r.tagName(), "v0.32.1");
}

TEST(GHAPIRelease, prerelease) {
    const auto j = nlohmann::json::parse(releasesData);
    gh::api::Release r0, r1;
    gh::api::from_json(j[0], r0);
    gh::api::from_json(j[1], r1);
    EXPECT_FALSE(r0.prerelease());
    EXPECT_TRUE(r1.prerelease());
}

TEST(GHAPIRelease, htmlUrl) {
    const auto j = nlohmann::json::parse(releasesData)[0];
    gh::api::Release r;
    gh::api::from_json(j, r);
    ASSERT_EQ(r.htmlUrl(), "https://github.com/black-sliver/PopTracker/releases/tag/v0.32.1");
}

TEST(GHAPIRelease, hasAssetUrl) {
    const auto j = nlohmann::json::parse(releasesData)[0];
    gh::api::Release r;
    gh::api::from_json(j, r);
    ASSERT_TRUE(r.hasAssetUrl("https://github.com/black-sliver/PopTracker/releases/download/v0.32.1/PopTracker-0.32.1-x86_64.AppImage"));
    EXPECT_TRUE(r.hasAssetUrl("https://github.com/black-sliver/PopTracker/releases/download/v0.32.1/PopTracker-0.32.1-x86_64.AppImage.minisig"));
    EXPECT_FALSE(r.hasAssetUrl("https://github.com/black-sliver/PopTracker/releases/download/v0.32.1/PopTracker-0.32.1-x86_64.AppImage.blerf"));
}

TEST(GHAPIRelease, assets) {
    const auto j = nlohmann::json::parse(releasesData);
    gh::api::Release r0, r1;
    gh::api::from_json(j[0], r0);
    gh::api::from_json(j[1], r1);
    EXPECT_GT(r0.assets().size(), 0u);
    EXPECT_EQ(r1.assets().size(), 0u);
}

TEST(GHAPIRelease, body)
{
    const auto j = nlohmann::json::parse(releasesData)[0];
    gh::api::Release r;
    gh::api::from_json(j, r);
    EXPECT_GT(r.body().length(), 0u);
}

TEST(GHAPIRelease, bodyContainsPubKey)
{
    // this mirrors the code in poptracker.cpp - TODO: extract to a common location for testing instead
    const auto j = nlohmann::json::parse(releasesData)[0];
    gh::api::Release rls;
    gh::api::from_json(j, rls);

    bool fileNameMatch = false;
    bool fileContentMatch = false;

    fs::path pubKeyDir = fs::app_path() / "key";
    if (!fs::is_directory(pubKeyDir))
        pubKeyDir = fs::current_path() / "key";
    for (auto& entry: fs::directory_iterator(pubKeyDir)) {
        if (entry.is_regular_file()) {
            if (rls.bodyContains(entry.path().filename())) {
                fileNameMatch = true;
            }
            std::string keyData;
            if (readFile(entry.path(), keyData)) {
                const auto p1 = keyData.find('\n');
                if (p1 != std::string::npos) {
                    const auto p2 = keyData.find_first_of("\r\n", p1 + 1);
                    keyData = keyData.substr(
                        p1 + 1,
                        p2 == std::string::npos ? std::string::npos : (p2 - p1 - 1)
                    );
                    EXPECT_FALSE(keyData.empty());
                    EXPECT_TRUE(keyData.find_first_of("\r\n") == std::string::npos);
                    if (rls.bodyContains(keyData)) {
                        fileContentMatch = true;
                    }
                } else {
                    ADD_FAILURE() << "could not parse" << sanitize_print(entry.path());
                }
            } else {
                ADD_FAILURE() << "could not read" << sanitize_print(entry.path());
            }
        }
        if (fileNameMatch && fileContentMatch)
            break;
    }
    EXPECT_TRUE(fileNameMatch);
    EXPECT_TRUE(fileContentMatch);
}
