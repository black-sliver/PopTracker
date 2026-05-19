#include <gtest/gtest.h>
#include "../../src/core/fileutil.h"
#include "../../src/core/fs.h"
#include "../../src/core/jsonutil.h"
#include "../../src/core/pack.h"
#include "../util/tempdir.hpp"


static fs::path my_realpath(const fs::path& path)
{
    fs::path res;
#if !defined WIN32 && !defined _WIN32
    if (char* tmp = realpath(path.c_str(), nullptr)) {
        res = tmp;
        free(tmp);
    }
#else
    if (auto tmp = _wfullpath(nullptr, path.c_str(), 1024)) {
        res = tmp;
        free(tmp);
    }
#endif
    return res;
}


TEST(PackTest, MissingNotInSearchPath) {
    Pack::addSearchPath("assets"); // ./packs/ may not exist
#if !defined WIN32 && !defined _WIN32
    const fs::path path = "/tmp/pack.zip";
#else
    const fs::path path = fs::u8path("C:\\pack.zip");
#endif
    EXPECT_FALSE(Pack::isInSearchPath(path));
}

TEST(PackTest, RelNotInSearchPath) {
    Pack::addSearchPath("assets"); // ./packs/ may not exist
    const fs::path path = fs::u8path("examples/async");
    EXPECT_FALSE(Pack::isInSearchPath(path));
}

TEST(PackTest, RelIsInSearchPath) {
    Pack::addSearchPath("examples");
    const fs::path path = fs::u8path("examples/async");
    EXPECT_TRUE(Pack::isInSearchPath(path));
}

TEST(PackTest, AbsIsInSearchPath) {
    Pack::addSearchPath("examples");
    const fs::path path = my_realpath(fs::u8path("examples/async"));
    EXPECT_TRUE(Pack::isInSearchPath(path));
}

TEST(PackTest, ReadFile) {
    const Pack pack("examples/rules_test");
    std::string s;
    ASSERT_TRUE(pack.ReadFile("items/items.json", s, false));
    auto j  = parse_jsonc(s);
    EXPECT_EQ(j.type(), nlohmann::json::value_t::array);
}

TEST(PackTest, ReadFilePartial) {
    const Pack pack("examples/rules_test");
    std::string full, part;
    ASSERT_TRUE(pack.ReadFile("items/items.json", full, false));
    ASSERT_GE(full.length(), 2u); // otherwise the test won't work
    ASSERT_TRUE(pack.ReadFile("items/items.json", part, false, full.length() / 2));
    EXPECT_EQ(part.length(), full.length() / 2);
    EXPECT_TRUE(full.find(part) == 0);
}

TEST(PackTest, ReadFileLimit) {
    const Pack pack("examples/rules_test");
    std::string full, part1, part2;
    ASSERT_TRUE(pack.ReadFile("items/items.json", full, false));
    ASSERT_TRUE(pack.ReadFile("items/items.json", part1, false, full.length()));
    ASSERT_TRUE(pack.ReadFile("items/items.json", part2, false, full.length() + 1));
    EXPECT_EQ(full, part1);
    EXPECT_EQ(full, part2);
}

TEST(PackTest, ReadFileOverride) {
    const std::string testData = "TestReadFileOverride";
    const std::string testPackPath = "examples/rules_test";
    const std::string testPackUID = "rules_test-example";

    TempDir tempDir;
    const fs::path overridesDir = tempDir.tempPath();
    const fs::path overrideDir = overridesDir / testPackUID;
    ASSERT_TRUE(fs::create_directories(overrideDir));
    const fs::path overrideFile = overrideDir / "README.md";
    writeFile(overrideFile, testData);
    Pack::addOverrideSearchPath(overridesDir);
    const Pack pack(testPackPath);

    std::string s;
    ASSERT_TRUE(pack.ReadFile("README.md", s, true));
    EXPECT_EQ(s, testData);
}

TEST(PackTest, ReadFileOverridePartial) {
    const std::string testData = "ReadFileOverridePartial";
    const std::string testPackPath = "examples/rules_test";
    const std::string testPackUID = "rules_test-example";

    TempDir tempDir;
    const fs::path overridesDir = tempDir.tempPath();
    const fs::path overrideDir = overridesDir / testPackUID;
    ASSERT_TRUE(fs::create_directories(overrideDir));
    const fs::path overrideFile = overrideDir / "README.md";
    writeFile(overrideFile, testData);
    Pack::addOverrideSearchPath(overridesDir);
    const Pack pack(testPackPath);

    std::string full, part;
    ASSERT_TRUE(pack.ReadFile("README.md", full, true));
    ASSERT_TRUE(pack.ReadFile("README.md", part, true, full.length() / 2));
    EXPECT_EQ(part.length(), full.length() / 2);
    EXPECT_TRUE(full.find(part) == 0);
}

TEST(PackTest, ReadFileOverrideLimit) {
    const std::string testData = "ReadFileOverrideLimit";
    const std::string testPackPath = "examples/rules_test";
    const std::string testPackUID = "rules_test-example";

    TempDir tempDir;
    const fs::path overridesDir = tempDir.tempPath();
    const fs::path overrideDir = overridesDir / testPackUID;
    ASSERT_TRUE(fs::create_directories(overrideDir));
    const fs::path overrideFile = overrideDir / "README.md";
    writeFile(overrideFile, testData);
    Pack::addOverrideSearchPath(overridesDir);
    const Pack pack(testPackPath);

    std::string full, part1, part2;
    ASSERT_TRUE(pack.ReadFile("README.md", full, true));
    ASSERT_TRUE(pack.ReadFile("README.md", part1, true, full.length()));
    ASSERT_TRUE(pack.ReadFile("README.md", part2, true, full.length() + 1));
    EXPECT_EQ(full, testData);
    EXPECT_EQ(full, part1);
    EXPECT_EQ(full, part2);
}
