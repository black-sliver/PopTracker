#pragma once

#ifdef WITH_HTTP
#include "http.h"
#include <string>
#include <list>
#include <nlohmann/json.hpp>

#include "../core/fs.h"

class HTTPCache {
    typedef nlohmann::json json;
public:
    HTTPCache(asio::io_service *asio, const fs::path& cachefile, const fs::path& cachedir, const std::list<std::string>& httpDefaultHeaders={});
    HTTPCache(const HTTPCache& orig) = delete;
    virtual ~HTTPCache();

protected:
    void GetCached(const std::string& url, std::function<void(bool, std::string)> cb);
    static std::string GetRandomName(const std::string& suffix="", int len=12);

    asio::io_service *_asio = nullptr;
    fs::path _cachefile;
    fs::path _cachedir;
    std::list<std::string> _httpDefaultHeaders;
    json _cache = json::object();
    int _minAge = 60; // don't fetch if less than X seconds old
};

#else

class HTTPCache {

};

#endif // WITH_HTTP
