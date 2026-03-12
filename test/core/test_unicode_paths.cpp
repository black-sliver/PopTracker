#include <asio/ssl.hpp>
#include <gtest/gtest.h>
#include "../../src/core/assets.h"
#include "../../src/core/fileutil.h"
#include "../../src/core/fs.h"
#include "../../src/uilib/imghelper.h"

#ifdef _WIN32
#include <windows.h>
#endif


class UnicodePathsTest : public testing::Test {
protected:
    fs::path _temp;

    UnicodePathsTest()
    {
        // ReSharper disable once CppLocalVariableMayBeConst
        auto tmplPath = fs::temp_directory_path() / "poptracker-test-XXXXXX";
#ifdef _WIN32
        auto tmpl = tmplPath.wstring();
        if (_wmktemp_s(tmpl.data(), tmpl.length() + 1) != 0)
            throw new std::runtime_error("Failed to create temporary name");
        fs::path tempDir = tmpl.data();
        if (!fs::create_directories(tempDir))
            throw new std::runtime_error("Failed to create temporary directory");
        _temp = tempDir;
#else
        auto tmpl = tmplPath.string();
        char* temp = mkdtemp(tmpl.data());
        if (!temp)
            throw new std::runtime_error("Failed to create temporary directory");
        _temp = fs::u8path(temp);
#endif
    }

public:
    ~UnicodePathsTest() override
    {
        if (!_temp.empty())
            fs::remove_all(_temp);
    }
};

TEST_F(UnicodePathsTest, ReadWrite) {
    const auto path = _temp / fs::u8path("ä.txt");
    const std::string data = "test\n";
    ASSERT_TRUE(writeFile(path, data));
    std::string data2;
    ASSERT_TRUE(readFile(path, data2));
    EXPECT_EQ(data2, data);
}

TEST_F(UnicodePathsTest, LoadImage) {
    const auto path = _temp / fs::u8path("ä.png");
    fs::copy(asset("icon.png"), path);
    const auto surf = Ui::LoadImage(path);
    ASSERT_NE(surf, nullptr);
    SDL_FreeSurface(surf);
}

TEST_F(UnicodePathsTest, SSLCert) {
    const auto path = _temp / fs::u8path("ä.pem");
    fs::copy(asset("cacert.pem"), path);
    asio::ssl::context ctx(asio::ssl::context::sslv23);
    asio::error_code ec;
    ctx.load_verify_file(path.u8string(), ec);
    ASSERT_TRUE(!ec);
}
