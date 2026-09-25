#include <lauxlib.h>
#include <lua.h>
#include <gtest/gtest.h>
#include "../../src/core/fs.h"
#include "../../src/core/pack.h"
#include "../../src/core/tracker.h"


TEST(Tracker, GetLocationSection) {
    lua_State* L = luaL_newstate();
    Pack pack("examples/rules_test");
    pack.setVariant("var_at");
    Tracker tracker(&pack, L);
    const Location& fullLocation1 = tracker.getLocation("Example Area/Start");
    const Location& partialLocation1 = tracker.getLocation("Start", true);
    const LocationSection& fullSection1 = tracker.getLocationSection("Example Area/Start/@A");
    const LocationSection& partialSection1 = tracker.getLocationSection("Start/@A");
    const auto [fullLocation2, fullSection2] = tracker.getLocationAndSection("Example Area/Start/@A");
    const auto [partialLocation2, partialSection2] = tracker.getLocationAndSection("Start/@A");
    const auto [partialLocation3, partialSection3] = tracker.getLocationAndSection("@A");
    EXPECT_EQ(&fullLocation1, &fullLocation2);
    EXPECT_EQ(&fullLocation1, &partialLocation1);
    EXPECT_EQ(&fullLocation1, &partialLocation2);
    EXPECT_EQ(&fullLocation1, &partialLocation3);
    EXPECT_EQ(&fullSection1, &fullSection2);
    EXPECT_EQ(&fullSection1, &partialSection1);
    EXPECT_EQ(&fullSection1, &partialSection2);
    EXPECT_EQ(&fullSection1, &partialSection3);
    lua_close(L);
}

#define EXPECT_BAD_OPEN_LINK(tracker, url, warning) { \
    testing::internal::CaptureStderr(); \
    try { \
        EXPECT_FALSE(tracker.OpenLink(url)); \
    } catch (...) { \
        fprintf(stderr, "%s", testing::internal::GetCapturedStderr().c_str()); \
        throw; \
    } \
    const auto capture = testing::internal::GetCapturedStderr(); \
    EXPECT_TRUE(capture.find(warning) != std::string::npos) << "Expected \"" warning "\" on stderr, got " << capture; \
}

#define EXPECT_INVALID_OPEN_LINK(tracker, url) EXPECT_BAD_OPEN_LINK(tracker, url, "invalid")
#define EXPECT_UNSECURED_OPEN_LINK(tracker, url) EXPECT_BAD_OPEN_LINK(tracker, url, "unsecured")

TEST(OpenLink, InvalidLinks)
{
    lua_State* L = luaL_newstate();
    Pack pack("examples/rules_test");
    pack.setVariant("var_at");
    Tracker tracker(&pack, L);

    const std::string longString(2048, 'a');
    EXPECT_INVALID_OPEN_LINK(tracker, "https://www.example.com/" + longString);
    EXPECT_INVALID_OPEN_LINK(tracker, "https://\".example.com/");
    EXPECT_INVALID_OPEN_LINK(tracker, "https://www.example.com/^");

    lua_close(L);
}

TEST(OpenLink, UnsecuredSchema)
{
    lua_State* L = luaL_newstate();
    Pack pack("examples/rules_test");
    pack.setVariant("var_at");
    Tracker tracker(&pack, L);

    EXPECT_UNSECURED_OPEN_LINK(tracker, "http://www.example.com/");
    EXPECT_UNSECURED_OPEN_LINK(tracker, "ftp://www.example.com/");

    lua_close(L);
}

// TODO: mock MsgBox and ShellExecute and test message escaping and happy code paths
