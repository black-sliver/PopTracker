#include <gtest/gtest.h>
#include <asio/ssl.hpp>
#include "../src/core/assets.h"
#include "../src/core/fileutil.h"
#include "../src/core/fs.h"
#include "../src/uilib/imghelper.h"

#ifdef _WIN32
#include <windows.h>
#endif


class UnicodePathsTest : public testing::Test {
protected:
    fs::path _temp;

    UnicodePathsTest()
    {
        auto tmplPath = fs::temp_directory_path() / "poptracker-test-XXXXXX";
#ifdef _WIN32
        auto tmpl = tmplPath.wstring();
        wchar_t* temp = _wmktemp(tmpl.data());
        if (!temp)
            throw new std::runtime_error("Failed to create temporary name");
        if (_wmkdir(temp) != 0)
            throw new std::runtime_error("Failed to create temporary directory");
        _temp = fs::path(temp);
#else
        auto tmpl = tmplPath.string();
        char* temp = mkdtemp(tmpl.data());
        if (!temp)
            throw new std::runtime_error("Failed to create temporary directory");
        _temp = fs::u8path(temp);
#endif
    }

public:
    virtual ~UnicodePathsTest()
    {
        if (!_temp.empty())
            fs::remove_all(_temp);
    }
};

TEST_F(UnicodePathsTest, ReadWrite) {
    auto path = _temp / fs::u8path("ä.txt");
    std::string data = "test\n";
    ASSERT_TRUE(writeFile(path, data));
    std::string data2;
    ASSERT_TRUE(readFile(path, data2));
    EXPECT_EQ(data2, data);
}

TEST_F(UnicodePathsTest, LoadImage) {
    auto path = _temp / fs::u8path("ä.png");
    fs::copy(asset("icon.png"), path);
    auto surf = Ui::LoadImage(path);
    ASSERT_NE(surf, nullptr);
    SDL_FreeSurface(surf);
}

TEST_F(UnicodePathsTest, SSLCert) {
    auto path = _temp / fs::u8path("ä.pem");
    fs::copy(asset("cacert.pem"), path);
    asio::ssl::context ctx(asio::ssl::context::sslv23);
    asio::error_code ec;
    ctx.load_verify_file(path.u8string(), ec);
    ASSERT_TRUE(!ec);
}
