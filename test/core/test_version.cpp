#include <gtest/gtest.h>
#include "../../src/core/version.h"


TEST(VersionTest, CompareBasic) {
    EXPECT_TRUE(Version() < Version(0, 0, 1));
    EXPECT_FALSE(Version(1, 0, 0) < Version(1, 0, 0));
    EXPECT_FALSE(Version(1, 0, 0) > Version(1, 0, 0));
    EXPECT_TRUE(Version(2, 0, 0) > Version(1, 1, 1));
    EXPECT_TRUE(Version(1, 1, 0) > Version(1, 0, 1));
    EXPECT_TRUE(Version(1, 0, 1) > Version(1, 0, 0));
    EXPECT_TRUE(Version(0, 0, 2) > Version(0, 0, 1));
    EXPECT_FALSE(Version(1, 0, 0) < Version("1"));
    EXPECT_FALSE(Version(1, 0, 0) < Version("1.0"));
    EXPECT_FALSE(Version(1, 0, 0) < Version("1.0.0"));
    EXPECT_TRUE(Version("1.2.3-1something2") < Version("1.2.3-2something1"));
}

TEST(VersionTest, CompareSemVer) {
    // semver, no tests for numeric build
    EXPECT_TRUE(Version("1.0.0") < Version("2.0.0"));
    EXPECT_TRUE(Version("2.0.0") < Version("2.1.0"));
    EXPECT_TRUE(Version("2.1.0") < Version("2.1.1"));
    EXPECT_TRUE(Version("1.0.0-alpha") < Version("1.0.0"));
    EXPECT_TRUE(Version("1.0.0-alpha") < Version("1.0.0-alpha.1"));
    EXPECT_TRUE(Version("1.0.0-alpha.1") < Version("1.0.0-alpha.beta"));
    EXPECT_TRUE(Version("1.0.0-alpha.beta") < Version("1.0.0-beta"));
    EXPECT_TRUE(Version("1.0.0-alpha.beta") < Version("1.0.0-beta.2"));
    EXPECT_TRUE(Version("1.0.0-alpha.beta.2") < Version("1.0.0-beta.11"));
    EXPECT_TRUE(Version("1.0.0-alpha.beta.11") < Version("1.0.0-rc.1"));
    EXPECT_TRUE(Version("1.0.0-rc.1") < Version("1.0.0"));
}

TEST(VersionTest, CompareSemVerBuild) {
    EXPECT_FALSE(Version("1.0.0+1") < Version("1.0.0+2"));
    EXPECT_FALSE(Version("1.0.0+1") > Version("1.0.0+2"));
    EXPECT_FALSE(Version("1.0.0-alpha+1") < Version("1.0.0-alpha+2"));
    EXPECT_FALSE(Version("1.0.0-alpha+1") > Version("1.0.0-alpha+2"));
}

TEST(VersionTest, CompareNumericBuild) {
    // numeric build, not semver
    EXPECT_FALSE(Version(1, 0, 0) < Version(1, 0, 0, 0));
    EXPECT_FALSE(Version(1, 0, 0, 0) < Version(1, 0, 0));
    EXPECT_FALSE(Version(1, 0, 0) < Version(1, 0, 0, 0));
    EXPECT_FALSE(Version("1.0.0.0") < Version(1, 0, 0));
    EXPECT_FALSE(Version(1, 0, 0) < Version("1.0.0.0"));
    EXPECT_TRUE(Version("1.4.3.0") < Version("1.4.3.1"));
    EXPECT_FALSE(Version("1.2.3.5") < Version(1, 2, 3, 4));
    EXPECT_TRUE(Version("1.2.3.4") < Version(1, 2, 3, 5));
    EXPECT_TRUE(Version(1, 2, 3) < Version("1.2.3.4"));
}

TEST(VersionTest, ToString) {
    EXPECT_EQ(Version("1.2.3").to_string(), "1.2.3");
    EXPECT_EQ(Version("1.2.3.4").to_string(), "1.2.3.4");
    EXPECT_EQ(Version("1.2.3-alpha.4").to_string(), "1.2.3-alpha.4");
    EXPECT_EQ(Version("1.2.3+4").to_string(), "1.2.3+4");
}

TEST(VersionTest, Extra) {
    auto v1 = Version(1, 2, 3, "extra");
    auto v2 = Version("1.2.3-extra");
    EXPECT_FALSE(v1 < v2);
    EXPECT_FALSE(v2 < v1);
    EXPECT_EQ(v1.to_string(), v2.to_string());
}

TEST(VersionTest, InvalidExtra) {
    EXPECT_EQ(Version("1.2.3'").to_string(), "1.2.3");
    EXPECT_EQ(Version("1.2.3-'a").to_string(), "1.2.3-a");
    EXPECT_EQ(Version("1.2.3+'a").to_string(), "1.2.3+a");
}
