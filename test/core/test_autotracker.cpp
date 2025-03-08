#include <gtest/gtest.h>
#include "../../src/core/autotracker.h"


/// Test that using an invalid index on an AutoTracker instance does not crash
TEST(AutoTracker, BadIndex) {
    AutoTracker at("none", {});
    // for empty AutoTracker, all indices are invalid (Unavailable), for disable() -1 has special meaning "all"
    for (int index = -2; index < 1; index++) {
        at.disable(index);
        EXPECT_FALSE(at.enable(index));
        EXPECT_EQ(at.getState(index), AutoTracker::State::Unavailable);
        EXPECT_EQ(at.getName(index), "");
        EXPECT_EQ(at.getSubName(index), "");
        EXPECT_FALSE(at.cycle(index));
    }
}

/// Test that accessing members of an empty AutoTracker instance does not crash
TEST(AutoTracker, EmptyAccess) {
    AutoTracker at("none", {});
    EXPECT_FALSE(at.doStuff());
    EXPECT_EQ(at.getState(""), AutoTracker::State::Unavailable);
    EXPECT_EQ(at.getState("SNES"), AutoTracker::State::Unavailable);
    EXPECT_FALSE(at.isAnyMemoryConnected());
    EXPECT_FALSE(at.addWatch(0, 1));
    EXPECT_FALSE(at.removeWatch(0, 1));
    at.setInterval(1000);
    at.clearCache();
    for (int addr=0; addr<2; addr++) {
        EXPECT_TRUE(at.read(addr, 1).empty());
        // unfailable reads without memory return 0
        EXPECT_EQ(at.ReadUInt8(addr), 0);
        EXPECT_EQ(at.ReadUInt16(addr), 0);
        EXPECT_EQ(at.ReadUInt24(addr), 0);
        EXPECT_EQ(at.ReadUInt32(addr), 0);
        for (int offs=0; offs<2; offs++) {
            EXPECT_EQ(at.ReadU8(addr, offs), 0);
            EXPECT_EQ(at.ReadU16(addr, offs), 0);
            EXPECT_EQ(at.ReadU24(addr, offs), 0);
            EXPECT_EQ(at.ReadU32(addr, offs), 0);
        }
    }
    at.ReadVariable("");
    EXPECT_EQ(at.getAP(), nullptr);
    EXPECT_EQ(at.getAutotrackProvider(), nullptr);
    at.setSnesAddresses({});
    EXPECT_FALSE(at.sync());
}
