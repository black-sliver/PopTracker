#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tuple>
#include <string_view>
#include <nlohmann/json.hpp>
#include "../../src/core/jsonitem.h"

using namespace std;
using nlohmann::json;

#define ALL_ITEM_TYPES \
    "static", \
    "toggle", \
    "progressive", \
    "consumable", \
    "progressive_toggle", \
    "composite_toggle", \
    "toggle_badged"


typedef bool loops;
typedef bool allowed_disabled;
typedef std::tuple<std::string_view, loops, allowed_disabled> ItemTypeAndSubType;

/// used to test state and stage validity of empty/minimal items after state changes
class EmptyJsonItemState : public testing::TestWithParam<ItemTypeAndSubType>
{
};

TEST_P(EmptyJsonItemState, AlwaysPositiveAndBackToInitial) {
    auto jItem = json({
        {"name", "item"},
        {"type", std::get<0>(GetParam())},
        {"codes", "Code"},
        {"loop", std::get<1>(GetParam())},
        {"allow_disabled", std::get<2>(GetParam())},
    });
    auto item = JsonItem::FromJSON(jItem);
    int initialStage1 = item.getState();
    int initialStage2 = item.getActiveStage();
    ASSERT_GE(initialStage1, 0);
    ASSERT_GE(initialStage2, 0);
    EXPECT_FALSE(item.getCurrentName().empty());

    // forth and back - left, right, left, right should get all types back to initial state
    item.changeState(BaseItem::Action::Primary);
    ASSERT_GE(item.getState(), 0);
    ASSERT_GE(item.getActiveStage(), 0);
    EXPECT_FALSE(item.getCurrentName().empty());

    item.changeState(BaseItem::Action::Secondary);
    ASSERT_GE(item.getState(), 0);
    ASSERT_GE(item.getActiveStage(), 0);
    EXPECT_FALSE(item.getCurrentName().empty());

    item.changeState(BaseItem::Action::Primary);
    item.changeState(BaseItem::Action::Secondary);
    if (item.getType() != BaseItem::Type::CONSUMABLE) {
        // FIXME: consumable has inconsistent stage1
        EXPECT_EQ(item.getState(), initialStage1);
    }
    EXPECT_EQ(item.getActiveStage(), initialStage2);
    ASSERT_GE(item.getState(), 0);
    ASSERT_GE(item.getActiveStage(), 0);
    EXPECT_FALSE(item.getCurrentName().empty());

    item.changeState(BaseItem::Action::Next);
    ASSERT_GE(item.getState(), 0);
    ASSERT_GE(item.getActiveStage(), 0);
    EXPECT_FALSE(item.getCurrentName().empty());

    item.changeState(BaseItem::Action::Prev);
    if (item.getType() != BaseItem::Type::CONSUMABLE) {
        // FIXME: consumable has inconsistent stage1
        EXPECT_EQ(item.getState(), initialStage1);
    }
    EXPECT_EQ(item.getActiveStage(), initialStage2);
    ASSERT_GE(item.getState(), 0);
    ASSERT_GE(item.getActiveStage(), 0);
    EXPECT_FALSE(item.getCurrentName().empty());

    item.changeState(BaseItem::Action::Toggle);
    ASSERT_GE(item.getState(), 0);
    ASSERT_GE(item.getActiveStage(), 0);
    EXPECT_FALSE(item.getCurrentName().empty());

    item.changeState(BaseItem::Action::Toggle);
    if (item.getType() != BaseItem::Type::CONSUMABLE) {
        // FIXME: consumable has inconsistent stage1
        EXPECT_EQ(item.getState(), initialStage1);
    }
    EXPECT_EQ(item.getActiveStage(), initialStage2);
    ASSERT_GE(item.getState(), 0);
    ASSERT_GE(item.getActiveStage(), 0);
    EXPECT_FALSE(item.getCurrentName().empty());

    item.changeState(BaseItem::Action::Single);
    ASSERT_GE(item.getState(), 0);
    ASSERT_GE(item.getActiveStage(), 0);
    EXPECT_FALSE(item.getCurrentName().empty());

    item.changeState(BaseItem::Action::Single);
    if (item.getType() != BaseItem::Type::CONSUMABLE) {
        // FIXME: consumable has inconsistent stage1
        EXPECT_EQ(item.getState(), initialStage1);
    }
    EXPECT_EQ(item.getActiveStage(), initialStage2);
    ASSERT_GE(item.getState(), 0);
    ASSERT_GE(item.getActiveStage(), 0);
    EXPECT_FALSE(item.getCurrentName().empty());

    // back from initial state (right)
    item.changeState(BaseItem::Action::Secondary);
    ASSERT_GE(item.getState(), 0);
    ASSERT_GE(item.getActiveStage(), 0);
    EXPECT_FALSE(item.getCurrentName().empty());

    // back from initial state (back)
    item = JsonItem::FromJSON(jItem);
    item.changeState(BaseItem::Action::Prev);
    ASSERT_GE(item.getState(), 0);
    ASSERT_GE(item.getActiveStage(), 0);
    EXPECT_FALSE(item.getCurrentName().empty());
}

INSTANTIATE_TEST_SUITE_P(
    AllTypes,
    EmptyJsonItemState,
    testing::Combine(
        testing::Values(ALL_ITEM_TYPES),
        testing::Values(false, true),
        testing::Values(false, true)
    )
);

static auto jProgressiveNoDisabled = R"(
{
    "name": "item",
    "type": "progressive",
    "loop": false,
    "allow_disabled": false,
    "stages": [
        {
            "name": "stage1",
            "codes": "code1"
        },
        {
            "name": "stage2",
            "codes": "code2"
        },
        {
            "name": "stage3",
            "codes": "code3"
        }
    ]
}
)"_json;

/// used to test stage advancement of progressive items with allow_disabled = false
class ProgressiveJsonItemState : public testing::TestWithParam<loops>
{
};

class MockEventHandler {
public:
    MOCK_METHOD(void, onEvent, ());
};

/// tests that stage correctly advances for progressive to check validity of changeState in EmptyJsonItemState
TEST_P(ProgressiveJsonItemState, Advances) {
    auto jItem = jProgressiveNoDisabled;
    bool loop = GetParam();
    jItem["loop"] = loop;
    auto item = JsonItem::FromJSON(jItem);

    MockEventHandler mock;
    item.onChange += {&mock, [&mock](void*) {mock.onEvent();}};
    EXPECT_CALL(mock, onEvent()).Times(2);

    EXPECT_EQ(item.getState(), 1);
    EXPECT_EQ(item.getActiveStage(), 0);
    EXPECT_EQ(item.getCurrentName(), jItem["stages"][0]["name"]);

    EXPECT_TRUE(item.changeState(BaseItem::Action::Primary));

    EXPECT_EQ(item.getState(), 1);
    EXPECT_EQ(item.getActiveStage(), 1);
    EXPECT_EQ(item.getCurrentName(), jItem["stages"][1]["name"]);

    EXPECT_TRUE(item.changeState(BaseItem::Action::Primary));

    EXPECT_EQ(item.getState(), 1);
    EXPECT_EQ(item.getActiveStage(), 2);
    EXPECT_EQ(item.getCurrentName(), jItem["stages"][2]["name"]);

    int expectedStage = loop ? 0 : 2;
    bool expectedChanged = loop ? true : false; // item loops -> true, otherwise unchanged -> false

    EXPECT_CALL(mock, onEvent()).Times(expectedChanged ? 1 : 0);
    EXPECT_EQ(item.changeState(BaseItem::Action::Primary), expectedChanged);

    EXPECT_EQ(item.getState(), 1);
    EXPECT_EQ(item.getActiveStage(), expectedStage);
    EXPECT_EQ(item.getCurrentName(), jItem["stages"][expectedStage]["name"]);
}

/// tests that stage correctly advances backwards for progressive to check validity of changeState in EmptyJsonItemState
TEST_P(ProgressiveJsonItemState, AdvancesBackwards) {
    auto jItem = jProgressiveNoDisabled;
    bool loop = GetParam();
    jItem["loop"] = loop;
    jItem["initial_stage_idx"] = jItem["stages"].size() - 1;
    auto item = JsonItem::FromJSON(jItem);

    EXPECT_EQ(item.getState(), 1);
    EXPECT_EQ(item.getActiveStage(), 2);
    EXPECT_EQ(item.getCurrentName(), jItem["stages"][2]["name"]);

    EXPECT_TRUE(item.changeState(BaseItem::Action::Secondary));

    EXPECT_EQ(item.getState(), 1);
    EXPECT_EQ(item.getActiveStage(), 1);
    EXPECT_EQ(item.getCurrentName(), jItem["stages"][1]["name"]);

    EXPECT_TRUE(item.changeState(BaseItem::Action::Secondary));

    EXPECT_EQ(item.getState(), 1);
    EXPECT_EQ(item.getActiveStage(), 0);
    EXPECT_EQ(item.getCurrentName(), jItem["stages"][0]["name"]);

    int expectedStage = loop ? 2 : 0;
    bool expectedChanged = loop ? true : false; // item loops -> true, otherwise unchanged -> false

    EXPECT_EQ(item.changeState(BaseItem::Action::Secondary), expectedChanged);

    EXPECT_EQ(item.getState(), 1);
    EXPECT_EQ(item.getActiveStage(), expectedStage);
    EXPECT_EQ(item.getCurrentName(), jItem["stages"][expectedStage]["name"]);
}

INSTANTIATE_TEST_SUITE_P(
    LoopAndNoLoop,
    ProgressiveJsonItemState,
    testing::Values(false, true)
);
