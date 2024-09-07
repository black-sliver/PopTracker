#include <gtest/gtest.h>
#include <list>
#include <string>
#include "../../src/core/jsonutil.h"


using namespace std;


// Living standard :TM:
// only trailing empty fields should be dropped

TEST(CommaSplitTest, Empty) {
    EXPECT_EQ(commasplit<list>(""),
              list<string>({}));
}

TEST(CommaSplitTest, TrailingComma) {
    // Trailing empty values should be dropped
    EXPECT_EQ(commasplit<list>("a,"),
              list<string>({"a"}));
}

TEST(CommaSplitTest, LeadingEmpty) {
    // Leading empty values should be kept
    EXPECT_EQ(commasplit<list>(",b"),
              list<string>({"", "b"}));
}

TEST(CommaSplitTest, MiddleEmpty) {
    // Empty values in the middle should be kept
    EXPECT_EQ(commasplit<list>("a,,b"),
              list<string>({"a", "", "b"}));
}

TEST(CommaSplitTest, TrailingWhitespace) {
    // If trailing whitespace results in trailing comma, it should behave the same as TrailingComma
    EXPECT_EQ(commasplit<list>("a, "),
              list<string>({"a"}));
}
