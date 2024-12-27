#include <gtest/gtest.h>
#include "../../src/core/fs.h"


TEST(FsTest, U8OneByte) {
    const char s[] = "a";
    static_assert(sizeof(s) == 2);
    EXPECT_EQ(fs::u8path(s).u8string(), s);
}

TEST(FsTest, U8TwoBytes) {
    const char s[] = "ä";
    static_assert(sizeof(s) == 3);
    EXPECT_EQ(fs::u8path(s).u8string(), s);
}

TEST(FsTest, U8ThreeBytes) {
    const char s[] = "シ";
    static_assert(sizeof(s) == 4);
    EXPECT_EQ(fs::u8path(s).u8string(), s);
}

TEST(FsTest, U8FourBytes) {
    const char s[] = "😀";
    static_assert(sizeof(s) == 5);
    EXPECT_EQ(fs::u8path(s).u8string(), s);
}

static fs::path my_realpath(const fs::path& path)
{
    fs::path res;
#if !defined WIN32 && !defined _WIN32
    char* tmp = realpath(path.c_str(), NULL);
    if (tmp) {
        res = tmp;
        free(tmp);
    }
#else
    auto tmp = _wfullpath(nullptr, path.c_str(), 1024);
    if (tmp) {
        auto cmp = [tmp](const fs::path& p) { return wcsicmp(tmp, p.c_str()) == 0; };
        res = tmp;
        free(tmp);
    }
#endif
    return res;
}

TEST(FsTest, SubPath) {
    EXPECT_TRUE(fs::is_sub_path(fs::path("assets") / "icon.png", "assets"));
}

TEST(FsTest, SubPathAbs) {
    EXPECT_TRUE(fs::is_sub_path(my_realpath("assets") / "icon.png", "assets"));
}

TEST(FsTest, SubPathAbsAbs) {
    EXPECT_TRUE(fs::is_sub_path(my_realpath("assets") / "icon.png", my_realpath("assets")));
}

TEST(FsTest, NotSubPath) {
    EXPECT_FALSE(fs::is_sub_path(fs::path("assets") / "icon.png", "doc"));
}

TEST(FsTest, NotSubPathAbs) {
    EXPECT_FALSE(fs::is_sub_path(my_realpath("assets") / "icon.png", "doc"));
}

TEST(FsTest, NotSubPathAbsAbs) {
    EXPECT_FALSE(fs::is_sub_path(my_realpath("assets") / "icon.png", my_realpath("doc")));
}

TEST(FsTest, IdenticalSubPath) {
    EXPECT_TRUE(fs::is_sub_path(fs::path("assets") / "icon.png", fs::path("assets") / "icon.png"));
}

TEST(FsTest, MissingSubPath) {
    EXPECT_FALSE(fs::is_sub_path(fs::path("missing") / "file", "missing"));
}

TEST(FsTest, AppPath) {
#if defined __APPLE__ or defined _WIN32 or defined __linux__
    EXPECT_FALSE(fs::app_path().empty());
#else
    (void)fs::app_path();
#endif
}

TEST(FsTest, HomePath) {
#if defined __APPLE__ or defined _WIN32 or defined __linux__
    EXPECT_FALSE(fs::home_path().empty());
#else
    (void)fs::home_path();
#endif
}

TEST(FsTest, DocumentsPath) {
#if defined __APPLE__ or defined _WIN32 or defined __linux__
    EXPECT_FALSE(fs::documents_path().empty());
#else
    (void)fs::documents_path();
#endif
}
