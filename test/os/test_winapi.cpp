// test random assumptions about the Windows API
#ifdef _WIN32

#include <gtest/gtest.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>


// Assume we get terminating NUL when we don't supply len

TEST(WinAPI, MultiByteToWideCharNoLenReturnValue1) {
    auto res = MultiByteToWideChar(CP_UTF8, 0, "test", -1, nullptr, 0);
    EXPECT_EQ(res, 5);
}

TEST(WinAPI, MultiByteToWideCharNoLenReturnValue2) {
    wchar_t buf[5];
    auto res = MultiByteToWideChar(CP_UTF8, 0, "test", -1, buf, 5);
    EXPECT_EQ(res, 5);
}

TEST(WinAPI, MultiByteToWideCharNoLenReturnValue3) {
    wchar_t buf[5];
    auto res = MultiByteToWideChar(CP_UTF8, 0, "test", -1, buf, 4);
    EXPECT_EQ(res, 0);
}

// Assume we don't get terminating NUL when we supply len

TEST(WinAPI, MultiByteToWideCharWithLenReturnValue1) {
    auto res = MultiByteToWideChar(CP_UTF8, 0, "test", 4, nullptr, 0);
    EXPECT_EQ(res, 4);
}

TEST(WinAPI, MultiByteToWideCharWithLenReturnValue2) {
    wchar_t buf[5];
    auto res = MultiByteToWideChar(CP_UTF8, 0, "test", 4, buf, 5);
    EXPECT_EQ(res, 4);
}

TEST(WinAPI, MultiByteToWideCharWithLenReturnValue3) {
    wchar_t buf[5];
    auto res = MultiByteToWideChar(CP_UTF8, 0, "test", 4, buf, 3);
    EXPECT_EQ(res, 0);
}

TEST(WinAPI, HasSEE_MASK_NOZONECHECKS) {
#ifndef SEE_MASK_NOZONECHECKS
    #error "SEE_MASK_NOZONECHECKS is not defined"
#else
    EXPECT_NE(SEE_MASK_NOZONECHECKS, 0) << "SEE_MASK_NOZONECHECKS has an unexpected value";
#endif
}

#endif
