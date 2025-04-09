#include <memory>
#include <string>
#include <string_view>
#include <gtest/gtest.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <nlohmann/json.hpp>
#include "../../src/core/jsonitem.h"
#include "../../src/core/tracker.h"

using namespace std;
using nlohmann::json;

class JsonItemToggleBadgedTest : public ::testing::TestWithParam<string_view> {
protected:
    lua_State* L;
    unique_ptr<Pack> pack;
    unique_ptr<Tracker> tracker;
    JsonItem* baseItem;
    JsonItem* badgeItem;

    JsonItemToggleBadgedTest() {
        L = luaL_newstate();
        pack = make_unique<Pack>("test/core/data");
        tracker = make_unique<Tracker>(pack.get(), L);
        auto jsonFile = "jsonitem_toggle_badged_" + string(GetParam()) + ".json";
        tracker->AddItems(jsonFile);
        baseItem = tracker->FindObjectForCode("base").jsonItem;
        badgeItem = tracker->FindObjectForCode("badge").jsonItem;
    }

    virtual ~JsonItemToggleBadgedTest() {
        lua_close(L);
    }
};

TEST_P(JsonItemToggleBadgedTest, CanProvideCode) {
    EXPECT_TRUE(baseItem->canProvideCode("base"));
    EXPECT_FALSE(baseItem->canProvideCode("badge"));
    EXPECT_TRUE(badgeItem->canProvideCode("badge"));
    EXPECT_FALSE(badgeItem->canProvideCode("base"));
}

TEST_P(JsonItemToggleBadgedTest, ClickBadgeProvidesCode) {
    std::string badgeID = badgeItem->getID();

    EXPECT_EQ(tracker->ProviderCountForCode("base"), 0);
    EXPECT_EQ(tracker->ProviderCountForCode("badge"), 0);
    tracker->changeItemState(badgeID, BaseItem::Action::Primary);
    EXPECT_EQ(tracker->ProviderCountForCode("base"), 1);
    EXPECT_EQ(tracker->ProviderCountForCode("badge"), 0);
    tracker->changeItemState(badgeID, BaseItem::Action::Secondary);
    EXPECT_EQ(tracker->ProviderCountForCode("base"), 1);
    EXPECT_EQ(tracker->ProviderCountForCode("badge"), 1);
}

TEST_P(JsonItemToggleBadgedTest, ClickBaseProvidesCode) {
    std::string baseID = baseItem->getID();

    EXPECT_EQ(tracker->ProviderCountForCode("base"), 0);
    EXPECT_EQ(tracker->ProviderCountForCode("badge"), 0);
    tracker->changeItemState(baseID, BaseItem::Action::Primary);
    EXPECT_EQ(tracker->ProviderCountForCode("base"), 1);
    EXPECT_EQ(tracker->ProviderCountForCode("badge"), 0);
    tracker->changeItemState(baseID, BaseItem::Action::Secondary);
    EXPECT_EQ(tracker->ProviderCountForCode("base"), 0);
    EXPECT_EQ(tracker->ProviderCountForCode("badge"), 0);
}

INSTANTIATE_TEST_SUITE_P(
    ItemOrder,
    JsonItemToggleBadgedTest,
    testing::Values("base_first", "badge_first"),
    [](const testing::TestParamInfo<JsonItemToggleBadgedTest::ParamType>& info) {
        return std::string(info.param);
    }
);
