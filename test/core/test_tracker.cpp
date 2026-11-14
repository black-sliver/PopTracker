#include <lauxlib.h>
#include <lua.h>
#include <gtest/gtest.h>
#include "../../src/core/fs.h"
#include "../../src/core/pack.h"
#include "../../src/core/tracker.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <cstdint> // uintptr_t
#include <gmock/gmock.h>
#include <gmock-win32.h>
#include <windows.h>
#include <shellapi.h> // ShellExecute
#include <winuser.h> // MessageBox
#endif


TEST(Tracker, GetLocationSection) {
    lua_State* L = luaL_newstate();
    Pack pack("examples/rules_test");
    pack.setVariant("var_at");
    Tracker tracker(&pack, L);
    const Location& fullLocation1 = tracker.getLocation("Example Area/Start");
    const Location& partialLocation1 = tracker.getLocation("Start", true);
    const LocationSection& fullSection1 = tracker.getLocationSection("Example Area/Start/@A");
    const LocationSection& partialSection1 = tracker.getLocationSection("Start/@A");
    const auto [fullLocation2, fullSection2] = tracker.getLocationAndSection("Example Area/Start/@A");
    const auto [partialLocation2, partialSection2] = tracker.getLocationAndSection("Start/@A");
    const auto [partialLocation3, partialSection3] = tracker.getLocationAndSection("@A");
    EXPECT_EQ(&fullLocation1, &fullLocation2);
    EXPECT_EQ(&fullLocation1, &partialLocation1);
    EXPECT_EQ(&fullLocation1, &partialLocation2);
    EXPECT_EQ(&fullLocation1, &partialLocation3);
    EXPECT_EQ(&fullSection1, &fullSection2);
    EXPECT_EQ(&fullSection1, &partialSection1);
    EXPECT_EQ(&fullSection1, &partialSection2);
    EXPECT_EQ(&fullSection1, &partialSection3);
    lua_close(L);
}

#define EXPECT_BAD_OPEN_LINK(tracker, url, warning) { \
    testing::internal::CaptureStderr(); \
    try { \
        EXPECT_FALSE(tracker.OpenLink(url)); \
    } catch (...) { \
        fprintf(stderr, "%s", testing::internal::GetCapturedStderr().c_str()); \
        throw; \
    } \
    const auto capture = testing::internal::GetCapturedStderr(); \
    EXPECT_TRUE(capture.find(warning) != std::string::npos) << "Expected \"" warning "\" on stderr, got " << capture; \
}

#define EXPECT_INVALID_OPEN_LINK(tracker, url) EXPECT_BAD_OPEN_LINK(tracker, url, "invalid")
#define EXPECT_UNSECURED_OPEN_LINK(tracker, url) EXPECT_BAD_OPEN_LINK(tracker, url, "unsecured")

TEST(OpenLink, InvalidLinks)
{
    lua_State* L = luaL_newstate();
    Pack pack("examples/rules_test");
    pack.setVariant("var_at");
    Tracker tracker(&pack, L);

    const std::string longString(2048, 'a');
    EXPECT_INVALID_OPEN_LINK(tracker, "https://www.example.com/" + longString);
    EXPECT_INVALID_OPEN_LINK(tracker, "https://\".example.com/");
    EXPECT_INVALID_OPEN_LINK(tracker, "https://www.example.com/^");

    lua_close(L);
}

TEST(OpenLink, UnsecuredSchema)
{
    lua_State* L = luaL_newstate();
    Pack pack("examples/rules_test");
    pack.setVariant("var_at");
    Tracker tracker(&pack, L);

    EXPECT_UNSECURED_OPEN_LINK(tracker, "http://www.example.com/");
    EXPECT_UNSECURED_OPEN_LINK(tracker, "ftp://www.example.com/");

    lua_close(L);
}

// TODO: also test message escaping

#ifdef _WIN32
using ::testing::_;

MOCK_STDCALL_FUNC(HINSTANCE, ShellExecuteA, HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);
MOCK_STDCALL_FUNC(HINSTANCE, ShellExecuteW, HWND, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, INT);
MOCK_STDCALL_FUNC(BOOL, ShellExecuteExA, SHELLEXECUTEINFOA*);
MOCK_STDCALL_FUNC(BOOL, ShellExecuteExW, SHELLEXECUTEINFOW*);
MOCK_STDCALL_FUNC(int, MessageBoxA, HWND, LPCSTR, LPCSTR, UINT);
MOCK_STDCALL_FUNC(int, MessageBoxW, HWND, LPCWSTR, LPCWSTR, UINT);

TEST(OpenLink, CallsShellExecuteA) {
    lua_State* L = luaL_newstate();
    Pack pack("examples/rules_test");
    pack.setVariant("var_at");
    Tracker tracker(&pack, L);

    // mock ShellExecuteA to return success (>32)
    // we currently use ShellExecuteA, not W, not Ex, but mock all in case we switch
    static_assert(sizeof(HINSTANCE) == sizeof(uintptr_t));
    constexpr HINSTANCE h42 = static_cast<HINSTANCE>(static_cast<uintptr_t>(42));
    ON_MODULE_FUNC_CALL(ShellExecuteA, _, _, _, _, _, _).WillByDefault(testing::Return(h42));
    ON_MODULE_FUNC_CALL(ShellExecuteW, _, _, _, _, _, _).WillByDefault(testing::Return(h42));
    ON_MODULE_FUNC_CALL(ShellExecuteExA, _).WillByDefault(testing::Return(0));
    ON_MODULE_FUNC_CALL(ShellExecuteExW, _).WillByDefault(testing::Return(0));
    EXPECT_MODULE_FUNC_CALL(ShellExecuteA).Times(1);
    EXPECT_MODULE_FUNC_CALL(ShellExecuteW).Times(0);
    EXPECT_MODULE_FUNC_CALL(ShellExecuteExA).Times(0);
    EXPECT_MODULE_FUNC_CALL(ShellExecuteExW).Times(0);
    // mock messagebox to say "Yes" (Open Link)
    // we currently use MessageBoxW, not A, but mock A as well to verify
    ON_MODULE_FUNC_CALL(MessageBoxA, _, _, _, _).WillByDefault(testing::Return(IDYES));
    ON_MODULE_FUNC_CALL(MessageBoxW, _, _, _, _).WillByDefault(testing::Return(IDYES));
    EXPECT_MODULE_FUNC_CALL(MessageBoxA).Times(0);
    EXPECT_MODULE_FUNC_CALL(MessageBoxW).Times(1);

    // run the function
    const bool res = tracker.OpenLink("https://poptracker.github.io");
    BYPASS_MOCKS(EXPECT_TRUE(res));

    // stop mocking
    RESTORE_MODULE_FUNC(ShellExecuteA);
    RESTORE_MODULE_FUNC(ShellExecuteW);
    RESTORE_MODULE_FUNC(ShellExecuteExA);
    RESTORE_MODULE_FUNC(ShellExecuteExW);
    RESTORE_MODULE_FUNC(MessageBoxA);
    RESTORE_MODULE_FUNC(MessageBoxW);
    // free lua state before teardown
    lua_close(L);
    // check calls
    VERIFY_AND_CLEAR_MODULE_FUNC(ShellExecuteA);
    VERIFY_AND_CLEAR_MODULE_FUNC(ShellExecuteW);
    VERIFY_AND_CLEAR_MODULE_FUNC(ShellExecuteExA);
    VERIFY_AND_CLEAR_MODULE_FUNC(ShellExecuteExW);
    VERIFY_AND_CLEAR_MODULE_FUNC(MessageBoxA);
    VERIFY_AND_CLEAR_MODULE_FUNC(MessageBoxW);
}
#endif
