#ifndef _HTTP_HTTPCACHE_H
#define _HTTP_HTTPCACHE_H


#include "http.h"
#include <string>
#include <list>
#include <nlohmann/json.hpp>


class HTTPCache {
    typedef nlohmann::json json;
public:
    HTTPCache(asio::io_service *asio, const std::string& cachefile, const std::string& cachedir, const std::list<std::string>& httpDefaultHeaders={});
    HTTPCache(const HTTPCache& orig) = delete;
    virtual ~HTTPCache();

protected:
    void GetCached(const std::string& url, std::function<void(bool, std::string)> cb);
    static std::string GetRandomName(const std::string& suffix="", int len=12);

    asio::io_service *_asio = nullptr;
    std::string _cachefile;
    std::string _cachedir;
    std::list<std::string> _httpDefaultHeaders;
    json _cache = json::object();
    int _minAge = 60; // don't fetch if less than X seconds old
};

#endif /* _HTTP_HTTPCACHE_H */
