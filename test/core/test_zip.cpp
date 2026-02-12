#include <cassert>
#include <cstdint>
#include <gtest/gtest.h>
#include "../fuzz.hpp"
#include "../../src/core/fileutil.h"
#include "../../src/core/zip.h"
#include "../util/fmemopen.hpp"

// sample data:
// see ./tools/make_zip.py
static auto SAMPLE_ZIP_PATH = "test/core/data/sample.zip";
static auto DEFLATE64_ZIP_PATH = "test/core/data/deflate64.zip";
static auto HDR64_ZIP_PATH = "test/core/data/hdr64.zip";

// compression methods:
// 0 = store, 8 = deflated, everything else unsupported

static inline void putU32(char* dst, const uint32_t u32)
{
    dst[0] = static_cast<char>((u32 >> 0) & 0xff);
    dst[1] = static_cast<char>((u32 >> 8) & 0xff);
    dst[2] = static_cast<char>((u32 >> 16) & 0xff);
    dst[3] = static_cast<char>((u32 >> 24) & 0xff);
}

static void makeZip(const std::string_view data, std::string& out)
{
    constexpr uint32_t startOfCompressed = 31; // 18 + 4 + 4 + 5
    const uint32_t compressedSize = data.length() > 2u ? data.length() - 3u : 0u;
    const uint32_t startOfCentralDirectory = startOfCompressed + compressedSize;
    constexpr uint32_t sizeOfCentralDirectory = 47; // 20 + 4 + 4 + 19
    const uint32_t startOfEnd = startOfCentralDirectory + sizeOfCentralDirectory;
    constexpr uint32_t sizeOfEnd = 22; // 12 + 4 + 4 + 2
    const uint32_t totalSize = startOfEnd + sizeOfEnd;
    out.resize(totalSize);
    assert(out.length() == totalSize);
    memset(out.data(), 0, out.size());
    // "local file header"
    out[0] = '\x50'; // magic number
    out[1] = '\x4b';
    out[2] = '\x03';
    out[3] = '\x04';
    out[4] = '\x0a'; // version needed
    out[8] = '\x08'; // compression method (deflate)
    putU32(out.data() + 18, compressedSize); // compressed size
    memcpy(out.data() + 22, data.data(), data.size() >= 3 ? 3 : data.size()); // uncompressed size
    out[26] = '\x01'; // file name len
    out[30] = 'a'; // file name ("a")
    // "data"
    memcpy(out.data() + startOfCompressed, data.data() + 3, data.size() >= 3 ? data.size() - 3 : 0);
    // "central directory file header"
    out[startOfCentralDirectory + 0] = '\x50'; // magic number
    out[startOfCentralDirectory + 1] = '\x4b';
    out[startOfCentralDirectory + 2] = '\x01';
    out[startOfCentralDirectory + 3] = '\x02';
    out[startOfCentralDirectory + 4] = '\x3f'; // version created
    out[startOfCentralDirectory + 5] = '\x03';
    out[startOfCentralDirectory + 6] = '\x0a'; // version needed
    out[startOfCentralDirectory + 10] = '\x08'; // compression method (deflate)
    putU32(out.data() + startOfCentralDirectory + 20, compressedSize); // compressed size
    memcpy(out.data() + startOfCentralDirectory + 24, data.data(), data.size() >= 3 ? 3 : data.size()); // uncompressed size
    out[startOfCentralDirectory + 28] = '\x01'; // file name len
    out[startOfCentralDirectory + 46] = 'a'; // file name ("a")
    // "end of central directory record"
    out[startOfEnd + 0] = '\x50'; // magic number
    out[startOfEnd + 1] = '\x4b';
    out[startOfEnd + 2] = '\x05';
    out[startOfEnd + 3] = '\x06';
    out[startOfEnd + 8] = '\x01'; // number of central directory records on this disk
    out[startOfEnd + 10] = '\x01'; // total number of central directory records
    putU32(out.data() + startOfEnd + 12, sizeOfCentralDirectory); // size of central directory
    putU32(out.data() + startOfEnd + 16, startOfCentralDirectory); // start of central directory
}

TEST(Zip, List)
{
    Zip zip(SAMPLE_ZIP_PATH);
    ASSERT_TRUE(zip.isValid());
    const auto items = zip.list(false);
    EXPECT_EQ(items.size(), 3u) <<  "expected 3 top level items in zip";
    for (const auto&[type, _]: items) {
        EXPECT_EQ(type, Zip::EntryType::DIR) << "expected all top level items be folders";
    }
}

TEST(Zip, ListRecursive)
{
    Zip zip(SAMPLE_ZIP_PATH);
    ASSERT_TRUE(zip.isValid());
    const auto items = zip.list(true);
    EXPECT_EQ(items.size(), 11u) <<  "expected 11 total items in zip";
    for (const auto&[type, name]: items) {
        if (name.rfind(".txt") != std::string::npos) {
            EXPECT_EQ(type, Zip::EntryType::FILE) << "expected .txt to be a file";
        } else {
            EXPECT_EQ(type, Zip::EntryType::DIR) << "expected non-.txt to be a dir";
        }
    }
}

TEST(Zip, ListFolder)
{
    Zip zip(SAMPLE_ZIP_PATH);
    ASSERT_TRUE(zip.isValid());
    zip.setDir("1");
    const auto items = zip.list(false);
    EXPECT_EQ(items.size(), 3u) <<  "expected 3 total items in folder \"/1\"";
}

TEST(Zip, Missing)
{
    Zip zip(SAMPLE_ZIP_PATH);
    ASSERT_TRUE(zip.isValid());
    std::string out, err;
    ASSERT_FALSE(zip.readFile("missing", out, err));
    EXPECT_EQ(out, "");
    EXPECT_EQ(err, "No such file in zip");
}

TEST(Zip, FileIsFolder)
{
    Zip zip(SAMPLE_ZIP_PATH);
    ASSERT_TRUE(zip.isValid());
    std::string out, err;
    ASSERT_FALSE(zip.readFile("1/", out, err));
    EXPECT_EQ(out, "");
    EXPECT_EQ(err, "Is directory");
}

TEST(Zip, Deflated)
{
    Zip zip(SAMPLE_ZIP_PATH);
    ASSERT_TRUE(zip.isValid());
    std::string data1;
    std::string data9;
    EXPECT_TRUE(zip.readFile("1/empty.txt", data1));
    EXPECT_TRUE(zip.readFile("9/empty.txt", data9));
    EXPECT_EQ(data1, data9);
    EXPECT_EQ(data1.length(), 0u);
    EXPECT_TRUE(zip.readFile("1/very_short.txt", data1));
    EXPECT_TRUE(zip.readFile("9/very_short.txt", data9));
    EXPECT_EQ(data1, data9);
    EXPECT_EQ(data1.length(), 4u);
    EXPECT_TRUE(zip.readFile("1/short.txt", data1));
    EXPECT_TRUE(zip.readFile("9/short.txt", data9));
    EXPECT_EQ(data1, data9);
    EXPECT_EQ(data1.length(), 8u * 1024u);
}

TEST(Zip, Stored)
{
    Zip zip(SAMPLE_ZIP_PATH);
    ASSERT_TRUE(zip.isValid());
    std::string data0;
    EXPECT_TRUE(zip.readFile("0/empty.txt", data0));
    EXPECT_EQ(data0.length(), 0u);
    EXPECT_TRUE(zip.readFile("0/very_short.txt", data0));
    EXPECT_EQ(data0.length(), 4u);
}

TEST(Zip, Deflate64)
{
    Zip zip(DEFLATE64_ZIP_PATH);
    ASSERT_TRUE(zip.isValid());
    const auto items = zip.list(true);
    ASSERT_EQ(items.size(), 1u) <<  "expected entry";
    std::string err;
    std::string data;
    if (zip.readFile("-", data, err)) {
        // if deflate64 is implemented, the file should be 64 bytes long
        EXPECT_EQ(data.length(), 64u);
    } else {
        // if deflate64 is unsupported, we should get a readable error for it
        EXPECT_EQ(err, "unsupported method");
    }
}

TEST(Zip, Hdr64)
{
    // tests support for ZIP64 file header
    Zip zip(HDR64_ZIP_PATH);
    ASSERT_TRUE(zip.isValid());
    const auto items = zip.list(true);
    ASSERT_EQ(items.size(), 1u) <<  "expected entry";
    std::string err;
    std::string data = "not empty";
    ASSERT_TRUE(zip.readFile("empty.txt", data, err));
    EXPECT_EQ(data.length(), 0u);
    EXPECT_EQ(err.length(), 0u);
}

TEST(Zip, Fopen)
{
    // test the constructor that takes a FILE*
    std::string zipData;
    makeZip("", zipData);
    FILE* fakeFile = fmemopen(zipData.data(), zipData.length(), "rb");
    Zip zip(fakeFile);
    std::string data;
    EXPECT_TRUE(zip.readFile("a", data));
    fclose(fakeFile);
}

static void FuzzWholeZip(const std::string_view data)
{
    FILE* fakeFile = fmemopen(const_cast<char*>(data.data()), data.length(), "rb");
    Zip zip(fakeFile);
    std::string out;
    for (const auto&[type, name]: zip.list(true)) {
        assert(type == Zip::EntryType::FILE || type == Zip::EntryType::DIR);
        if (type == Zip::EntryType::FILE) {
            zip.readFile(name, out);
        }
    }
    zip.readFile("a", out);
    fclose(fakeFile);
}

static void FuzzDeflateZipEntry(const std::string_view data)
{
    static std::string zipData;
    makeZip(data, zipData);
    FuzzWholeZip(zipData);
}

static void FuzzSetDir(const std::string_view data)
{
    Zip zip(SAMPLE_ZIP_PATH);
    assert(zip.isValid());
    zip.setDir(std::string(data));
    for (const auto&[type, _]: zip.list(true)) {
        (void)type;
        assert(type == Zip::EntryType::FILE || type == Zip::EntryType::DIR);
    }
    for (const auto&[type, _]: zip.list(false)) {
        (void)type;
        assert(type == Zip::EntryType::FILE || type == Zip::EntryType::DIR);
    }
}

static void FuzzFilename(const std::string_view data)
{
    Zip zip(SAMPLE_ZIP_PATH);
    assert(zip.isValid());
    std::string tmp;
    zip.readFile(std::string(data), tmp);
}

// TODO: also add test for "full" ZIP64 (CD, EOCD)

FUZZ(FuzzWholeZip);
FUZZ(FuzzDeflateZipEntry);
FUZZ(FuzzSetDir);
FUZZ(FuzzFilename);
