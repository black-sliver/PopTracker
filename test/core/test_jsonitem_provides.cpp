#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "../../src/core/jsonitem.h"

using namespace std;
using nlohmann::json;


static auto jStatic = R"(
{
    "name": "item",
    "type": "static",
    "codes": "Code"
}
)"_json;

TEST(JsonItemProvidesTest, StaticCanProvideCode) {
    auto item = JsonItem::FromJSON(jStatic);
    EXPECT_TRUE(item.canProvideCode("Code"));
    EXPECT_FALSE(item.canProvideCode("Not"));
}

#ifdef JSONITEM_CI_QUIRK
TEST(JsonItemProvidesTest, StaticCanProvideCodeCI) {
    auto item = JsonItem::FromJSON(jStatic);
    EXPECT_TRUE(item.canProvideCode("codE"));
    EXPECT_TRUE(item.canProvideCodeLower("code"));
    EXPECT_FALSE(item.canProvideCodeLower("not"));
}
#endif

TEST(JsonItemProvidesTest, StaticProvidesCode) {
    auto item = JsonItem::FromJSON(jStatic);
    EXPECT_EQ(item.providesCode("Code"), 1);
    EXPECT_EQ(item.providesCode("Not"), 0);

    #if 0  // FIXME: we CAN currently force static to off using setState
    item.setState(0);
    EXPECT_EQ(item.providesCode("Code"), 1);
    EXPECT_EQ(item.providesCode("Not"), 0);
    #endif

    item.changeState(BaseItem::Action::Toggle);
    EXPECT_EQ(item.providesCode("Code"), 1);
    EXPECT_EQ(item.providesCode("Not"), 0);
    item.changeState(BaseItem::Action::Toggle);
    EXPECT_EQ(item.providesCode("Code"), 1);
    EXPECT_EQ(item.providesCode("Not"), 0);
}

#ifdef JSONITEM_CI_QUIRK
TEST(JsonItemProvidesTest, StaticProvidesCodeCI) {
    auto item = JsonItem::FromJSON(jStatic);
    EXPECT_EQ(item.providesCode("codE"), 1);
    EXPECT_EQ(item.providesCodeLower("code"), 1);
}
#endif

static auto jToggle = R"(
{
    "name": "item",
    "type": "toggle",
    "codes": "code1,CODE2"
}
)"_json;

TEST(JsonItemProvidesTest, ToggleCanProvideCode) {
    auto item = JsonItem::FromJSON(jToggle);
    EXPECT_TRUE(item.canProvideCode("code1"));
    EXPECT_TRUE(item.canProvideCode("CODE2"));
    EXPECT_FALSE(item.canProvideCode("code3"));
}

#ifdef JSONITEM_CI_QUIRK
TEST(JsonItemProvidesTest, ToggleCanProvideCodeCI) {
    auto item = JsonItem::FromJSON(jToggle);
    EXPECT_TRUE(item.canProvideCode("CODE1"));
    EXPECT_TRUE(item.canProvideCode("code2"));
    EXPECT_TRUE(item.canProvideCodeLower("code1"));
    EXPECT_TRUE(item.canProvideCodeLower("code2"));
}
#endif

TEST(JsonItemProvidesTest, ToggleProvidesCode) {
    auto item = JsonItem::FromJSON(jToggle);
    EXPECT_EQ(item.providesCode("code1"), 0);
    EXPECT_EQ(item.providesCode("CODE2"), 0);
    EXPECT_EQ(item.providesCode("code3"), 0);
    item.setState(1);
    EXPECT_EQ(item.providesCode("code1"), 1);
    EXPECT_EQ(item.providesCode("CODE2"), 1);
    EXPECT_EQ(item.providesCode("code3"), 0);
}

#ifdef JSONITEM_CI_QUIRK
TEST(JsonItemProvidesTest, ToggleProvidesCodeCI) {
    auto item = JsonItem::FromJSON(jToggle);
    EXPECT_EQ(item.providesCode("CODE1"), 0);
    EXPECT_EQ(item.providesCode("code2"), 0);
    EXPECT_EQ(item.providesCodeLower("code2"), 0);
    item.setState(1);
    EXPECT_EQ(item.providesCode("CODE1"), 1);
    EXPECT_EQ(item.providesCode("code2"), 1);
    EXPECT_EQ(item.providesCodeLower("code2"), 1);
}
#endif

static auto jStagedSeparate = R"(
{
    "name": "item",
    "type": "staged",
    "codes": "Base",
    "stages": [
        {
            "inherit_codes": false,
            "codes": "Stage1"
        },
        {
            "inherit_codes": false,
            "codes": "Stage2"
        }
    ]
}
)"_json;

TEST(JsonItemProvidesTest, StagedCanProvideCode) {
    // NOTE: currently Base works for canProvide so it can be used to FindObjectForCode; change this?
    auto item = JsonItem::FromJSON(jStagedSeparate);
    EXPECT_TRUE(item.canProvideCode("Base"));
    EXPECT_TRUE(item.canProvideCode("Stage1"));
    EXPECT_TRUE(item.canProvideCode("Stage2"));
    EXPECT_FALSE(item.canProvideCode("Not"));
}

#ifdef JSONITEM_CI_QUIRK
TEST(JsonItemProvidesTest, StagedCanProvideCodeCI) {
    // NOTE: currently Base works for canProvide so it can be used to FindObjectForCode; change this?
    auto item = JsonItem::FromJSON(jStagedSeparate);
    EXPECT_TRUE(item.canProvideCode("BASE"));
    EXPECT_TRUE(item.canProvideCode("STAGE1"));
    EXPECT_TRUE(item.canProvideCodeLower("stage2"));
    EXPECT_FALSE(item.canProvideCodeLower("not"));
}
#endif

TEST(JsonItemProvidesTest, StagedProvidesCode) {
    // NOTE: currently Base is never provided for staged because it's not really valid for staged; change this?
    auto item = JsonItem::FromJSON(jStagedSeparate);
    EXPECT_EQ(item.providesCode("Base"), 0);
    EXPECT_EQ(item.providesCode("Stage1"), 0);
    EXPECT_EQ(item.providesCode("Stage2"), 0);
    item.setState(1, -1);
    EXPECT_EQ(item.providesCode("Base"), 0);
    EXPECT_EQ(item.providesCode("Stage1"), 1);
    EXPECT_EQ(item.providesCode("Stage2"), 0);
    item.setState(1, 1);
    EXPECT_EQ(item.providesCode("Base"), 0);
    EXPECT_EQ(item.providesCode("Stage1"), 0);
    EXPECT_EQ(item.providesCode("Stage2"), 1);
}

#ifdef JSONITEM_CI_QUIRK
TEST(JsonItemProvidesTest, StagedProvidesCodeCI) {
    auto item = JsonItem::FromJSON(jStagedSeparate);
    item.setState(1, 0);
    EXPECT_EQ(item.providesCode("STAGE1"), 1);
    EXPECT_EQ(item.providesCodeLower("stage1"), 1);
    item.setState(1, 1);
    EXPECT_EQ(item.providesCode("STAGE1"), 0);
    EXPECT_EQ(item.providesCodeLower("stage1"), 0);
}
#endif

static auto jStagedInherits = R"(
{
    "name": "item",
    "type": "staged",
    "codes": "Base",
    "stages": [
        {
            "inherit_codes": true,
            "codes": "Stage1"
        },
        {
            "inherit_codes": true,
            "codes": "Stage2"
        }
    ]
}
)"_json;

TEST(JsonItemProvidesTest, StagedInheritsCanProvideCode) {
    // NOTE: currently Base works for canProvide so it can be used to FindObjectForCode; change this?
    auto item = JsonItem::FromJSON(jStagedInherits);
    EXPECT_TRUE(item.canProvideCode("Base"));
    EXPECT_TRUE(item.canProvideCode("Stage1"));
    EXPECT_TRUE(item.canProvideCode("Stage2"));
    EXPECT_FALSE(item.canProvideCode("Not"));
}

#ifdef JSONITEM_CI_QUIRK
TEST(JsonItemProvidesTest, StagedInheritsCanProvideCodeCI) {
    // NOTE: currently Base works for canProvide so it can be used to FindObjectForCode; change this?
    auto item = JsonItem::FromJSON(jStagedInherits);
    EXPECT_TRUE(item.canProvideCode("BASE"));
    EXPECT_TRUE(item.canProvideCode("STAGE1"));
    EXPECT_TRUE(item.canProvideCodeLower("stage2"));
    EXPECT_FALSE(item.canProvideCodeLower("not"));
}
#endif

TEST(JsonItemProvidesTest, StagedInheritsProvidesCode) {
    // NOTE: currently Base is never provided for staged because it's not really valid for staged; change this?
    auto item = JsonItem::FromJSON(jStagedInherits);
    EXPECT_EQ(item.providesCode("Base"), 0);
    EXPECT_EQ(item.providesCode("Stage1"), 0);
    EXPECT_EQ(item.providesCode("Stage2"), 0);
    item.setState(1, -1);
    EXPECT_EQ(item.providesCode("Base"), 0);
    EXPECT_EQ(item.providesCode("Stage1"), 1);
    EXPECT_EQ(item.providesCode("Stage2"), 0);
    item.setState(1, 1);
    EXPECT_EQ(item.providesCode("Base"), 0);
    EXPECT_EQ(item.providesCode("Stage1"), 1);
    EXPECT_EQ(item.providesCode("Stage2"), 1);
}

#ifdef JSONITEM_CI_QUIRK
TEST(JsonItemProvidesTest, StagedInheritsProvidesCodeCI) {
    auto item = JsonItem::FromJSON(jStagedInherits);
    item.setState(1, 0);
    EXPECT_EQ(item.providesCode("STAGE1"), 1);
    EXPECT_EQ(item.providesCodeLower("stage1"), 1);
}
#endif
