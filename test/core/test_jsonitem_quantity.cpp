#include <limits.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <lua.h>
#include <lauxlib.h>
#include "../../src/core/jsonitem.h"

using namespace std;
using nlohmann::json;


typedef std::tuple<int, int> MinMax;

/// used to test initial and assigned quantity is bound to expected limits
class ConsumableQuantityLimit : public testing::TestWithParam<MinMax> {
protected:
    int minQ = 0; ///< parametrized min
    int maxQ = 0; ///< parametrized max
    int insideQ = 0; ///< value that should always be valid to assign
    const int belowQ = -100; ///< value that should always be below min
    const int aboveQ = 100; ///< value that should always be above max
    json jItem; ///< parametrized json with min_quantity and max_qyuantity set

    void SetUp() override
    {
        minQ = std::get<0>(GetParam());
        maxQ = std::get<1>(GetParam());
        insideQ = hasUpperLimit() ? ((minQ + maxQ) / 2) : aboveQ;
        jItem = json({
            {"name", "item"},
            {"type", "consumable"},
            {"codes", "Code"},
            {"min_quantity", minQ},
            {"max_quantity", maxQ},
        });
        ASSERT_GT(minQ, belowQ) << "min parameter has to be > " << belowQ << " for the tests below";
        ASSERT_LT(maxQ, aboveQ) << "max parameter has to be < " << aboveQ << " for the tests below";
    }

    /// returns true if quantity has an upper limit, false otherwise
    bool hasUpperLimit() const
    {
        return maxQ >= minQ;
    }
};

static void luaSetValue(JsonItem& item, const char* key, int value)
{
    lua_State* L = luaL_newstate();
    ASSERT_TRUE(L);
    JsonItem::Lua_Register(L);
    item.Lua_Push(L);
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
    lua_pop(L, 1);
    lua_close(L);
}

TEST_P(ConsumableQuantityLimit, DefaultInitialValue) {
    const auto item = JsonItem::FromJSON(jItem);
    if (minQ >= 0) // value has to be >= min
        EXPECT_EQ(item.getCount(), minQ);
    else if (maxQ < 0 && maxQ >= minQ) // value has to be <= max
        EXPECT_EQ(item.getCount(), maxQ);
    else // 0 is valid
        EXPECT_EQ(item.getCount(), 0);
}

TEST_P(ConsumableQuantityLimit, InitialBelowLimit) {
    jItem["initial_quantity"] = belowQ;
    const auto item = JsonItem::FromJSON(jItem);
    EXPECT_EQ(item.getCount(), minQ);
}

TEST_P(ConsumableQuantityLimit, InitialAboveLimit) {
    if (!hasUpperLimit()) // no upper limit
        return;
    jItem["initial_quantity"] = aboveQ;
    const auto item = JsonItem::FromJSON(jItem);
    EXPECT_EQ(item.getCount(), maxQ);
}

TEST_P(ConsumableQuantityLimit, InitialInsideLimit) {
    jItem["initial_quantity"] = insideQ;
    const auto item = JsonItem::FromJSON(jItem);
    EXPECT_EQ(item.getCount(), insideQ);
}

TEST_P(ConsumableQuantityLimit, AssignBelowLimit) {
    auto item = JsonItem::FromJSON(jItem);
    luaSetValue(item, "AcquiredCount", belowQ);
    EXPECT_EQ(item.getCount(), minQ);
}

TEST_P(ConsumableQuantityLimit, AssignAboveLimit) {
    auto item = JsonItem::FromJSON(jItem);
    luaSetValue(item, "AcquiredCount", aboveQ);
    if (hasUpperLimit())
        EXPECT_EQ(item.getCount(), maxQ);
    else
        EXPECT_EQ(item.getCount(), aboveQ);
}

TEST_P(ConsumableQuantityLimit, AssignInsideLimit) {
    auto item = JsonItem::FromJSON(jItem);
    luaSetValue(item, "AcquiredCount", insideQ);
    EXPECT_EQ(item.getCount(), insideQ);
}

TEST_P(ConsumableQuantityLimit, DecrementBelowLimit) {
    auto item = JsonItem::FromJSON(jItem);
    for (int i = item.getCount(); i >= minQ - 1; --i)
        item.changeState(BaseItem::Action::Secondary);
    EXPECT_EQ(item.getCount(), minQ);
}

TEST_P(ConsumableQuantityLimit, IncrementAboveLimit) {
    if (maxQ < minQ)
        return; // there is no limit
    auto item = JsonItem::FromJSON(jItem);
    for (int i = item.getCount(); i <= maxQ + 1; ++i)
        item.changeState(BaseItem::Action::Primary);
    EXPECT_EQ(item.getCount(), maxQ);
}

INSTANTIATE_TEST_SUITE_P(
    Limits,
    ConsumableQuantityLimit,
    testing::Values(
        std::make_tuple(0, 0),
        std::make_tuple(0, 7),
        std::make_tuple(0, -7),
        std::make_tuple(5, 0),
        std::make_tuple(5, 3),
        std::make_tuple(5, 7),
        std::make_tuple(5, -7),
        std::make_tuple(-5, 0),
        std::make_tuple(-5, -3),
        std::make_tuple(-5, -7),
        std::make_tuple(-5, 7)
    ),
    [](const testing::TestParamInfo<ConsumableQuantityLimit::ParamType>& info) {
        std::string minS = std::to_string(std::get<0>(info.param));
        std::string maxS = std::to_string(std::get<1>(info.param));
        if (minS[0] == '-')
            minS[0] = 'n';
        if (maxS[0] == '-')
            maxS[0] = 'n';
        return "min_" + minS + "_max_" + maxS;
    }
);

TEST(ConsumableDefaultQuantity, InitialBelowLimit) {
    // for default min/max, quantity should not be able to go below 0
    auto jItem = json({
        {"name", "item"},
        {"type", "consumable"},
        {"codes", "Code"},
        {"initial_quantity", -2},
    });
    const auto item = JsonItem::FromJSON(jItem);
    EXPECT_EQ(item.getCount(), 0);
}

TEST(ConsumableDefaultQuantity, MinBelowZero) {
    // for min < 0, initial should still default to 0
    // this could fail if default max_quantity is not correct
    auto jItem = json({
        {"name", "item"},
        {"type", "consumable"},
        {"codes", "Code"},
        {"min_quantity", -100},
    });
    const auto item = JsonItem::FromJSON(jItem);
    EXPECT_EQ(item.getCount(), 0);
}
