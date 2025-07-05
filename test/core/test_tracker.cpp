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
