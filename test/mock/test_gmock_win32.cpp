// test that gmock-win32 works
#ifdef _WIN32
#include <gmock/gmock.h>
#include <gmock-win32.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


MOCK_STDCALL_FUNC(DWORD, GetCurrentThreadId);


TEST(GmockWin32Test, MockWinApi) {
    ON_MODULE_FUNC_CALL(GetCurrentThreadId).WillByDefault(testing::Return(42U));
    EXPECT_MODULE_FUNC_CALL(GetCurrentThreadId).Times(1);

    const auto tid1 = ::GetCurrentThreadId();
    BYPASS_MOCKS(EXPECT_EQ(tid1, 42U)); // gtest uses GetCurrentThreadId, so we bypass mocks here

    RESTORE_MODULE_FUNC(GetCurrentThreadId);
    VERIFY_AND_CLEAR_MODULE_FUNC_EXPECTATIONS(GetCurrentThreadId);
}
#endif
