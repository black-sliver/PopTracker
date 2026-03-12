#include <string>
#include <gtest/gtest.h>
#include "../../src/core/fileutil.h"
#include "../../src/core/fs.h"
#include "../../src/core/sha256.h"
#include "../util/tempdir.hpp"


TempDir tempDir;

TEST(SHA256, NoFile) {
    const auto p = tempDir.tempPath();
    EXPECT_EQ(SHA256_File(p), "");
}

TEST(SHA256, EmptyFile) {
    const auto p = tempDir.tempPath();
    writeFile(p, "");
    ASSERT_EQ(SHA256_File(p), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    fs::remove(p);
}

TEST(SHA256, Len4095File) {
    const auto p = tempDir.tempPath();
    if (!writeFile(p, std::string(4095, 0)))
        throw new std::runtime_error("Failed to write file");
    EXPECT_EQ(SHA256_File(p), "2cae68411db14d6b340e650cd7e512a0d604379425f48e5a8ba846336777ff5c");
    fs::remove(p);
}

TEST(SHA256, Len4096File) {
    const auto p = tempDir.tempPath();
    if (!writeFile(p, std::string(4096, 0)))
        throw new std::runtime_error("Failed to write file");
    EXPECT_EQ(SHA256_File(p), "ad7facb2586fc6e966c004d7d1d16b024f5805ff7cb47c7a85dabd8b48892ca7");
    fs::remove(p);
}

TEST(SHA256, Len4097File) {
    const auto p = tempDir.tempPath();
    if (!writeFile(p, std::string(4097, 0)))
        throw new std::runtime_error("Failed to write file");
    EXPECT_EQ(SHA256_File(p), "b587fa297299ce9c602e58292b51379402bf7b1074f6b18679c2fb871c917ca8");
    fs::remove(p);
}
