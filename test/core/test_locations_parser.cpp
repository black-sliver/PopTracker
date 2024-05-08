#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "../../src/core/location.h"


using namespace std;
using nlohmann::json;


TEST(LocationsParserTest, LocationRulesInheritAndEmpty) {
    // TODO: validate indirectly by checcking accessibility/visibilty via Tracker
    const list<list<string>> parentAccessRules = {{"a"}};
    const list<list<string>> parentVisibilityRules = {{"b"}};
    json childNode = R"(
        {
            "name": "child",
            "access_rules": [],
            "visibility_rules": []
        }
    )"_json;

    auto location = Location::FromJSON(childNode, {}, parentAccessRules, parentVisibilityRules).front();
    EXPECT_EQ(location.getAccessRules(),
              parentAccessRules);
    EXPECT_EQ(location.getVisibilityRules(),
              parentVisibilityRules);
}

TEST(LocationsParserTest, SectionRulesInheritAndEmpty) {
    // TODO: validate indirectly by checcking accessibility/visibilty via Tracker
    json locationNode = R"(
        {
            "name": "location",
            "access_rules": [["a"]],
            "visibility_rules": [["b"]],
            "sections": [
                {
                    "name": "section",
                    "access_rules": [],
                    "visibility_rules": []
                }
            ]
        }
    )"_json;

    auto section = Location::FromJSON(locationNode, {}, {}, {}).front().getSections().front();
    EXPECT_EQ(section.getAccessRules(),
              list<list<string>>({{"a"}}));
    EXPECT_EQ(section.getVisibilityRules(),
              list<list<string>>({{"b"}}));
}
