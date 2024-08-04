#include <gtest/gtest.h>
#include "../../src/core/util.h"


TEST(SanitizeFilenameTest, InvalidSymbol) {
    EXPECT_EQ(sanitize_filename({"a\x00z", 3}), "a_z");
    EXPECT_EQ(sanitize_filename("ä"), "ä");
    EXPECT_EQ(sanitize_filename("a*b"), "a_b");
    EXPECT_EQ(sanitize_filename("a/b"), "a_b");
    EXPECT_EQ(sanitize_filename("a\\b"), "a_b");
}

TEST(SanitizeFilenameTest, Empty) {
    // allow empty filename
    EXPECT_EQ(sanitize_filename(""), "");
}

TEST(SanitizeDirTest, InvalidSymbol) {
    EXPECT_EQ(sanitize_dir({"a\x00z", 3}), "a_z");
    EXPECT_EQ(sanitize_dir("a\x01z"), "a_z");
    EXPECT_EQ(sanitize_dir("a\x80z"), "a_z");
    EXPECT_EQ(sanitize_dir("a*b"), "a_b");
    EXPECT_EQ(sanitize_dir("a/b"), "a_b");
    EXPECT_EQ(sanitize_dir("a\\b"), "a_b");
}

TEST(SanitizeDirTest, Empty) {
    // can't have an empty dirname
    EXPECT_EQ(sanitize_dir(""), "_");
}

TEST(SanitizeDirTest, NoCWD) {
    EXPECT_EQ(sanitize_dir("."), "_");
    EXPECT_EQ(sanitize_dir("./"), "._");
}

TEST(SanitizeDirTest, NoParent) {
    EXPECT_EQ(sanitize_dir(".."), "_");
    EXPECT_EQ(sanitize_dir("../"), ".._");
    EXPECT_EQ(sanitize_dir("..\\"), ".._");
}
