#include <gtest/gtest.h>
#include <string>
#include "../../src/core/fileutil.h"
#include "../../src/core/fs.h"
#include "../../src/core/sha256.h"

fs::path tempPath()
{
    auto tmplPath = fs::temp_directory_path() / "poptracker-test-XXXXXX";
    #ifdef _WIN32
        auto tmpl = tmplPath.wstring();
        wchar_t* temp = _wmktemp(tmpl.data());
        if (!temp)
            throw new std::runtime_error("Failed to create temporary name");
        return fs::path(temp);
    #else
        auto tmpl = tmplPath.string();
        char* temp = mktemp(tmpl.data());
        if (!temp)
            throw new std::runtime_error("Failed to create temporary directory");
        return fs::u8path(temp);
    #endif
}

TEST(SHA256, NoFile) {
    auto p = tempPath();
    EXPECT_EQ(SHA256_File(p), "");
}

TEST(SHA256, EmptyFile) {
    auto p = tempPath();
    writeFile(p, "");
    EXPECT_EQ(SHA256_File(p), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(SHA256, Len4095File) {
    auto p = tempPath();
    writeFile(p, std::string(4095, 0));
    EXPECT_EQ(SHA256_File(p), "2cae68411db14d6b340e650cd7e512a0d604379425f48e5a8ba846336777ff5c");
}

TEST(SHA256, Len4096File) {
    auto p = tempPath();
    writeFile(p, std::string(4096, 0));
    EXPECT_EQ(SHA256_File(p), "ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7");
}

TEST(SHA256, Len4097File) {
    auto p = tempPath();
    writeFile(p, std::string(4097, 0));
    EXPECT_EQ(SHA256_File(p), "b587fa297299ce9c602e58292b51379402bf7b1074f6b18679c2fb871c917ca8");
}
