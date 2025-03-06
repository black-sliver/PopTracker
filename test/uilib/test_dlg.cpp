#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../src/uilib/dlg.h"

using ::testing::_;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gmock-win32.h>


MOCK_STDCALL_FUNC(BOOL, GetOpenFileNameW, OPENFILENAMEW*);
MOCK_STDCALL_FUNC(BOOL, GetSaveFileNameW, OPENFILENAMEW*);


TEST(DlgTest, WindowsOpenFile) {
    fs::path dflt = "default";

    ON_MODULE_FUNC_CALL(GetOpenFileNameW, _).WillByDefault([&dflt](OPENFILENAMEW* pofn) {
      	// validate arguments passed to GetOpenFileNameW
        const auto& s = dflt.native();
        auto byteLen = sizeof(decltype(dflt)::value_type) * (s.length() + 1);
    	if (memcmp(pofn->lpstrFile, s.c_str(), byteLen) != 0)
        	return false;
        return true;
	});
    EXPECT_MODULE_FUNC_CALL(GetOpenFileNameW, _).Times(1);

  	fs::path out;
	bool res = Ui::Dlg::OpenFile("Title", dflt, { {"Any", {"*"}} }, out, false);
    EXPECT_TRUE(res);

    RESTORE_MODULE_FUNC(GetOpenFileNameW);
    VERIFY_AND_CLEAR_MODULE_FUNC_EXPECTATIONS(GetOpenFileNameW);
}

TEST(DlgTest, WindowsSaveFile) {
    fs::path dflt = "default";

    ON_MODULE_FUNC_CALL(GetSaveFileNameW, _).WillByDefault([&dflt](OPENFILENAMEW* pofn) {
      	// validate arguments passed to GetOpenFileNameW
        const auto& s = dflt.native();
        auto byteLen = sizeof(decltype(dflt)::value_type) * (s.length() + 1);
    	if (memcmp(pofn->lpstrFile, s.c_str(), byteLen) != 0)
        	return false;
        return true;
	});
    EXPECT_MODULE_FUNC_CALL(GetSaveFileNameW, _).Times(1);

  	fs::path out;
	bool res = Ui::Dlg::SaveFile("Title", dflt, { {"Any", {"*"}} }, out);
    EXPECT_TRUE(res);

    RESTORE_MODULE_FUNC(GetSaveFileNameW);
    VERIFY_AND_CLEAR_MODULE_FUNC_EXPECTATIONS(GetSaveFileNameW);
}

#endif // _WIN32
