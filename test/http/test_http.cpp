#include <string>
#include <gtest/gtest.h>
#include "../../src/core/assets.h"
#include "../../src/http/http.h"


using namespace std;


TEST(HTTP, SyncGet200) {
    std::string response;
    const int code = HTTP::Get("https://poptracker.github.io", response);
    EXPECT_EQ(code, 200);
    EXPECT_NE(response.find("<html"), string::npos);
}
