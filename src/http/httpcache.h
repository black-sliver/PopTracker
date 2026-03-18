#pragma once

#include <list>
#include <string>
#include <nlohmann/json.hpp>
#include "http.h"
#include "../core/fs.h"


class HTTPCache {
    typedef nlohmann::json json;
public:
    HTTPCache(asio::io_service *asio, fs::path cacheFile, fs::path cacheDir,
        const std::list<std::string>& httpDefaultHeaders={});
    HTTPCache(const HTTPCache& orig) = delete;
    virtual ~HTTPCache();

protected:
    void GetCached(const std::string& url, const std::function<void(bool, std::string)>& cb);

    static std::string GetRandomName(std::string_view suffix = "", int len=12);

    static std::string GetRandomName(const std::string& suffix, const int len=12)
    {
        return GetRandomName(std::string_view{suffix}, len);
    }

    static std::string GetRandomName(const char* suffix, const int len=12)
    {
        return GetRandomName(std::string_view{suffix}, len);
    }

    asio::io_service *_asio = nullptr;
    fs::path _cacheFile;
    fs::path _cacheDir;
    std::list<std::string> _httpDefaultHeaders;
    json _cache = json::object();
    int _minAge = 60; // don't fetch if less than X seconds old
};
