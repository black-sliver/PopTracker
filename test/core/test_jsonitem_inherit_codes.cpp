#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "../../src/core/jsonitem.h"

using namespace std;
using nlohmann::json;


/// test that progressive with inherit_codes follows documentation
/// NOTE: "inherit_codes": true omitted from last stage to test default
TEST(JsonItemInheritsCodesTest, DocumentationExample)
{
    using namespace std::literals::string_literals;

    auto jDocumentationExample = R"(
    {
        "name": "item",
        "type": "progressive",
        "allow_disabled": false,
        "stages": [
            {
                "name": "0",
                "codes": "a",
                "inherit_codes": false
            },
            {
                "name": "1",
                "codes": "b",
                "inherit_codes": false
            },
            {
                "name": "2",
                "codes": "c",
                "inherit_codes": true
            },
            {
                "name": "3",
                "codes": "d",
                "inherit_codes": true
            },
            {
                "name": "4",
                "codes": "e",
                "inherit_codes": false
            },
            {
                "name": "5",
                "codes": "f"
            }
        ]
    }
    )"_json;

    auto item = JsonItem::FromJSON(jDocumentationExample);
    const std::string allCodes = "abcdef";
    ASSERT_EQ(std::string{'a'}, std::string("a"));
    for (const auto& expected : {"a"s, "b"s, "bc"s, "bcd"s, "e"s, "ef"s}) {
        for (char c: allCodes) {
            EXPECT_EQ(item.providesCode({c}), expected.find(c) == std::string::npos ? 0 : 1)
            << "for " << c << " at stage " << item.getActiveStage() << " expected " << expected;
        }
        item.changeState(BaseItem::Action::Next);
    }
}
