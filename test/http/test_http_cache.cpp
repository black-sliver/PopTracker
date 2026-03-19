#include <string>
#include <gtest/gtest.h>
#include "../../src/http/http.h"
#include "../../src/http/httpcache.h"
#include "../util/tempdir.hpp"


using namespace std;

static TempDir tempDir;

const std::string redirectingUrl = "https://github.com/black-sliver/PopTracker/releases/latest";
const auto timeout = std::chrono::seconds(10);


class TestCache : public HTTPCache {
public:
    explicit TestCache(asio::io_context* ioContext)
        : HTTPCache(ioContext, tempDir.tempPath(), tempDir.tempPath())
    {
    }

    void TestGetCached(const std::string& uri, const std::function<void(bool, std::string)>& cb, const int redirectLimit)
    {
        GetCached(uri, cb, redirectLimit);
    }

    void TestGetCached(const std::string& uri, const std::function<void(bool, std::string)>& cb)
    {
        GetCached(uri, cb);
    }
};

TEST(HTTP, GetCachedNotFollowingRedirect) {
    asio::io_context ioContext;
    TestCache cache{&ioContext};
    bool done = false;
    bool ok = false;
    cache.TestGetCached(redirectingUrl, [&done, &ok](const bool res, const std::string&) {
        done = true;
        ok = res;
    }, 0);
    const size_t events = ioContext.run_for(timeout);
    EXPECT_GT(events, 1u);
    EXPECT_TRUE(done);
    EXPECT_FALSE(ok); // redirect is not OK
}

TEST(HTTP, GetCachedFollowRedirect) {
    asio::io_context ioContext;
    TestCache cache{&ioContext};

    bool done1 = false;
    bool ok1 = false;
    std::string response1;
    cache.TestGetCached(redirectingUrl, [&done1, &ok1, &response1](const bool ok, const std::string& r) {
        done1 = true;
        ok1 = ok;
        response1 = r;
    });
    const size_t events1 = ioContext.run_for(timeout);
    EXPECT_GT(events1, 1u);
    ASSERT_TRUE(done1);
    ASSERT_TRUE(ok1);
    EXPECT_TRUE(response1.length());

    ioContext.restart();
    bool done2 = false;
    bool ok2 = false;
    std::string response2;
    cache.TestGetCached(redirectingUrl, [&done2, &ok2, &response2](const bool ok, const std::string& r) {
        done2 = true;
        ok2 = ok;
        response2 = r;
    });
    const size_t events2 = ioContext.run_for(timeout);
    EXPECT_NE(events2, events1); // NOTE: this may not hold true
    ASSERT_TRUE(done2);
    EXPECT_TRUE(ok2);
    EXPECT_EQ(response2, response1);
}
