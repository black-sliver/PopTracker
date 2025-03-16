#include <gtest/gtest.h>
#include "../../src/core/fs.h"
#include "../../src/core/pack.h"


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
        res = tmp;
        free(tmp);
    }
#endif
    return res;
}


TEST(PackTest, MissingNotInSearchPath) {
    Pack::addSearchPath("assets"); // ./packs/ may not exist
#if !defined WIN32 && !defined _WIN32
    fs::path path = "/tmp/pack.zip";
#else
    fs::path path = fs::u8path("C:\\pack.zip");
#endif
    EXPECT_FALSE(Pack::isInSearchPath(path));
}

TEST(PackTest, RelNotInSearchPath) {
    Pack::addSearchPath("assets"); // ./packs/ may not exist
    fs::path path = fs::u8path("examples/async");
    EXPECT_FALSE(Pack::isInSearchPath(path));
}

TEST(PackTest, RelIsInSearchPath) {
    Pack::addSearchPath("examples");
    fs::path path = fs::u8path("examples/async");
    EXPECT_TRUE(Pack::isInSearchPath(path));
}

TEST(PackTest, AbsIsInSearchPath) {
    Pack::addSearchPath("examples");
    fs::path path = my_realpath(fs::u8path("examples/async"));
    EXPECT_TRUE(Pack::isInSearchPath(path));
}
