// tests parsing the update info from JSON responses

#include <gtest/gtest.h>

#include "../../lib/asio/include/asio/io_service.hpp"
#include "../../src/poptracker.h"
#include "../../src/appupdater/appupdater.hpp"
#include "../util/tempdir.hpp"


static asio::io_service ioService;
static TempDir tempDir;

static const std::string currentTag = std::string("v") + PopTracker::VERSION_STRING;
static const std::string validPubKeyBody = "Public key: `RWR/KVOoufNpOxFX4VZUW/gDaRnlr5tJkRIHNIvaC0qxbbYPTxOX22q9`";
const std::string owner = "black-sliver";
const std::string repo = "PopTracker";
const std::string baseUrl = "https://github.com/" + owner + "/" + repo + "/releases/download/";
const std::string currentTagUrl = baseUrl + currentTag + "/";


/// Fixture to run tests for both included and excluded prereleases via GetParam().
class AppUpdaterParseTest : public pop::AppUpdater, public testing::TestWithParam<bool> {
protected:
    AppUpdaterParseTest()
    : AppUpdater(ioService, {}, owner, repo, GetParam(),
        tempDir.tempPath(), tempDir.tempPath())
    {
    }
};


TEST_P(AppUpdaterParseTest, EmptyString)
{
    EXPECT_ANY_THROW({
        updateFromResponse("");
    });
}

TEST_P(AppUpdaterParseTest, InvalidJson)
{
    EXPECT_ANY_THROW({
        updateFromResponse("[");
    });
}

TEST_P(AppUpdaterParseTest, NotArray)
{
    EXPECT_ANY_THROW({
        updateFromResponse("{}");
    });
}

TEST_P(AppUpdaterParseTest, EmptyArray)
{
    EXPECT_FALSE(updateFromResponse("[]").has_value());
}

static std::string createDownloadUrl(const std::string& name, const std::string& tag)
{
    return baseUrl + tag + "/" + name;
}

static nlohmann::json createAsset(const std::string& name, const std::string& tag, std::string_view updatedAt)
{
    return {
        {"name", name},
        {"browser_download_url", createDownloadUrl(name, tag)},
        {"updated_at", updatedAt},
    };
}

TEST_P(AppUpdaterParseTest, AlreadyLatest)
{
    const std::string updatedAt = "2026-04-03T08:29:48Z";
    constexpr bool prerelease = false;
    const std::string assetName = "PopTracker-0.35.0-x86_64.AppImage";
    const std::string s = json{
        {
            {"html_url", "..."},
            {"body", validPubKeyBody},
            {"tag_name", currentTag},
            {"prerelease", prerelease},
            {"updated_at", updatedAt},
            {"assets", {
                createAsset(assetName, currentTag, updatedAt),
                createAsset(assetName + ".minisig", currentTag, updatedAt),
            }}
        }
    }.dump();

    EXPECT_FALSE(updateFromResponse(s).has_value());
}

TEST_P(AppUpdaterParseTest, ReleaseAvailable)
{
    const std::string newerVersion = "9999.99.99";
    const std::string newerTag = "v" + newerVersion;
    const std::string updatedAt = "2026-04-03T08:29:48Z";
    constexpr bool prerelease = false;
    const std::string assetName = "PopTracker-" + newerVersion + "-x86_64.AppImage";
    const std::string s = json{
        {
            {"html_url", "..."},
            {"body", validPubKeyBody},
            {"tag_name", newerTag},
            {"prerelease", prerelease},
            {"updated_at", updatedAt},
            {"assets", {
                createAsset(assetName, newerTag, updatedAt),
                createAsset(assetName + ".minisig", newerTag, updatedAt),
            }}
        }
    }.dump();

    const auto update = updateFromResponse(s);
    ASSERT_TRUE(update.has_value());
    EXPECT_EQ(update->versionStr, newerVersion);
    EXPECT_FALSE(update->browserUrl.empty());
    EXPECT_FALSE(update->assets.empty());
}

TEST_P(AppUpdaterParseTest, PrereleaseAvailable)
{
    const std::string newerVersion = "9999.99.99-rc1";
    const std::string newerTag = "v" + newerVersion;
    const std::string updatedAt = "2026-04-03T08:29:48Z";
    constexpr bool prerelease = true;
    const std::string assetName = "PopTracker-" + newerVersion + "-x86_64.AppImage";
    const std::string s = json{
        {
            {"html_url", "..."},
            {"body", validPubKeyBody},
            {"tag_name", newerTag},
            {"prerelease", prerelease},
            {"updated_at", updatedAt},
            {"assets", {
                createAsset(assetName, newerTag, updatedAt),
                createAsset(assetName + ".minisig", newerTag, updatedAt),
            }}
        }
    }.dump();

    const auto update = updateFromResponse(s);
    ASSERT_EQ(update.has_value(), GetParam());
    if (GetParam()) {
        EXPECT_EQ(update->versionStr, newerVersion);
        EXPECT_FALSE(update->browserUrl.empty());
        EXPECT_FALSE(update->assets.empty());
    }
}

// TODO: also test RC1 -> RC2 update even if prereleases are excluded, but this requires injection of current version

TEST_P(AppUpdaterParseTest, NotCompatible)
{
    const std::string newerVersion = "9999.99.99";
    const std::string newerTag = "v" + newerVersion;
    const std::string updatedAt = "2026-04-03T08:29:48Z";
    constexpr bool prerelease = false;
    const std::string s = json{
        {
            {"html_url", "..."},
            {"body", validPubKeyBody},
            {"tag_name", newerTag},
            {"prerelease", prerelease},
            {"updated_at", updatedAt},
            {"assets", json::array()}
        }
    }.dump();

    const auto update = updateFromResponse(s);
    ASSERT_TRUE(update.has_value());
    EXPECT_TRUE(update->versionStr.empty());
    EXPECT_TRUE(update->browserUrl.empty()); // url empty if update is not compatible
    EXPECT_TRUE(update->assets.empty());
}

TEST_P(AppUpdaterParseTest, NotAutoInstallable)
{
    const std::string newerVersion = "9999.99.99";
    const std::string newerTag = "v" + newerVersion;
    const std::string updatedAt = "2026-04-03T08:29:48Z";
    constexpr bool prerelease = false;
    const std::string assetName = "PopTracker-" + newerVersion + "-x86_64.AppImage";
    const std::string s = json{
        {
            {"html_url", "..."},
            {"body", ""},
            {"tag_name", newerTag},
            {"prerelease", prerelease},
            {"updated_at", updatedAt},
            {"assets", {
                createAsset(assetName, newerTag, updatedAt),
                createAsset(assetName + ".minisig", newerTag, updatedAt),
            }}
        }
    }.dump();

    const auto update = updateFromResponse(s);
    ASSERT_TRUE(update.has_value());
    EXPECT_EQ(update->versionStr, newerVersion);
    EXPECT_FALSE(update->browserUrl.empty());
    EXPECT_TRUE(update->assets.empty()); // assets empty if update can't be auto-installed
}

TEST_P(AppUpdaterParseTest, ReleaseAndPrerelease)
{
    const std::string newerVersion = "9999.99.98";
    const std::string evenNewerVersion = "9999.99.99-rc1";
    const std::string newerTag = "v" + newerVersion;
    const std::string evenNewerTag = "v" + evenNewerVersion;
    const std::string updatedAt = "2026-04-03T08:29:48Z";
    constexpr bool newerPrerelease = false;
    constexpr bool evenNewerPrerelease = true;
    const std::string newerAssetName = "PopTracker-" + newerVersion + "-x86_64.AppImage";
    const std::string evenNewerAssetName = "PopTracker-" + evenNewerVersion + "-x86_64.AppImage";
    const std::string newerBrowserUrl = "1"; // is not validated
    const std::string evenNewerBrowserUrl = "2";
    const std::string s = json{
        {
            {"html_url", evenNewerBrowserUrl},
            {"body", validPubKeyBody},
            {"tag_name", evenNewerTag},
            {"prerelease", evenNewerPrerelease},
            {"updated_at", updatedAt},
            {"assets", {
                createAsset(evenNewerAssetName, evenNewerTag, updatedAt),
                createAsset(evenNewerAssetName + ".minisig", evenNewerTag, updatedAt),
            }}
        },
        {
            {"html_url", newerBrowserUrl},
            {"body", validPubKeyBody},
            {"tag_name", newerTag},
            {"prerelease", newerPrerelease},
            {"updated_at", updatedAt},
            {"assets", {
                createAsset(newerAssetName, newerTag, updatedAt),
                createAsset(newerAssetName + ".minisig", newerTag, updatedAt),
            }}
        }
    }.dump();

    const auto update = updateFromResponse(s);
    ASSERT_TRUE(update.has_value());
    if (GetParam()) {
        EXPECT_EQ(update->versionStr, evenNewerVersion);
        EXPECT_EQ(update->browserUrl, evenNewerBrowserUrl);
        EXPECT_EQ(update->assets.at(0).browserDownloadUrl(), createDownloadUrl(evenNewerAssetName, evenNewerTag));
    } else {
        EXPECT_EQ(update->versionStr, newerVersion);
        EXPECT_EQ(update->browserUrl, newerBrowserUrl);
        EXPECT_EQ(update->assets.at(0).browserDownloadUrl(), createDownloadUrl(newerAssetName, newerTag));
    }
}

TEST_P(AppUpdaterParseTest, ReleaseAndNotAutoInstallable)
{
    const std::string newerVersion = "9999.99.98";
    const std::string evenNewerVersion = "9999.99.99";
    const std::string newerTag = "v" + newerVersion;
    const std::string evenNewerTag = "v" + evenNewerVersion;
    const std::string updatedAt = "2026-04-03T08:29:48Z";
    constexpr bool prerelease = false;
    const std::string newerAssetName = "PopTracker-" + newerVersion + "-x86_64.AppImage";
    const std::string evenNewerAssetName = "PopTracker-" + evenNewerVersion + "-x86_64.AppImage";
    const std::string newerBrowserUrl = "1"; // is not validated
    const std::string evenNewerBrowserUrl = "2";
    const std::string s = json{
        {
            {"html_url", evenNewerBrowserUrl},
            {"body", ""},
            {"tag_name", evenNewerTag},
            {"prerelease", prerelease},
            {"updated_at", updatedAt},
            {"assets", {
                createAsset(evenNewerAssetName, evenNewerTag, updatedAt),
                createAsset(evenNewerAssetName + ".minisig", evenNewerTag, updatedAt),
            }}
        },
        {
            {"html_url", newerBrowserUrl},
            {"body", validPubKeyBody},
            {"tag_name", newerTag},
            {"prerelease", prerelease},
            {"updated_at", updatedAt},
            {"assets", {
                createAsset(newerAssetName, newerTag, updatedAt),
                createAsset(newerAssetName + ".minisig", newerTag, updatedAt),
            }}
        }
    }.dump();

    const auto update = updateFromResponse(s);
    ASSERT_TRUE(update.has_value());
    EXPECT_EQ(update->versionStr, newerVersion);
    EXPECT_EQ(update->browserUrl, newerBrowserUrl);
    EXPECT_EQ(update->assets.at(0).browserDownloadUrl(), createDownloadUrl(newerAssetName, newerTag));
}


INSTANTIATE_TEST_SUITE_P(IncludePreprelease, AppUpdaterParseTest, testing::Values(false, true));
