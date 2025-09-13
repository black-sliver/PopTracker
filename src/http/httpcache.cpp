#include "httpcache.h"
#include "../core/fileutil.h"
#include "../core/util.h"
#include <random>


#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif


std::string HTTPCache::GetRandomName(const std::string& suffix, int len)
{
    std::string res;
    const char chars[] = "0123456789abcdef"
                         "ghijklmnopqrstuvwxyz";
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0,strlen(chars)-1);
    for (int n=0; n<len; n++) {
        res.append(1, chars[dist(rng)]);
    }
    res += suffix;
    return res;
}


HTTPCache::HTTPCache(asio::io_service *asio, const fs::path& cachefile, const fs::path& cachedir, const std::list<std::string>& httpDefaultHeaders)
    : _asio(asio), _cachefile(cachefile), _cachedir(cachedir), _httpDefaultHeaders(httpDefaultHeaders)
{
    fs::create_directories(_cachedir);
    std::string s;
    try {
        if (readFile(_cachefile, s)) {
            _cache = json::parse(s);
        }
    } catch (...) {
        printf("Could not load http cache!\n");
    }
    // TODO: clean out cache:
    // * delete entries older than 3months
    // * sync files <-> json
}

HTTPCache::~HTTPCache()
{
}

void HTTPCache::GetCached(const std::string& url, std::function<void(bool, std::string)> cb)
{
    auto headers = _httpDefaultHeaders;
    auto cacheIt = _cache.find(url);
    if (cacheIt != _cache.end()) {
        // use cache if timestamp is newer than _minAge and not in the future
        std::string s;
        if (_minAge != 0 && cacheIt.value()["file"].is_string() &&
                cacheIt.value()["timestamp"].is_number_unsigned() &&
                cacheIt.value()["timestamp"].get<time_t>() > time(NULL)-_minAge &&  // less than 60 seconds old
                cacheIt.value()["timestamp"].get<time_t>() < time(NULL)+30 &&  // less than 30 seconds in the future
                readFile(_cachedir / fs::u8path(cacheIt.value()["file"].get<std::string>()), s)) {
            cb(true, s);
            return;
        } else if (cacheIt.value()["etag"].is_string() && cacheIt.value()["file"].is_string() &&
                fs::is_regular_file(_cachedir / fs::u8path(cacheIt.value()["file"].get<std::string>()))) {
            headers.push_back("If-None-Match: " + cacheIt.value()["etag"].get<std::string>());
        } else {
            _cache.erase(cacheIt);
        }
    }
    if (!HTTP::GetAsync(*_asio, url, headers,
            [this, url, cb](int code, const std::string& r, HTTP::Headers h)
    {
        // TODO: follow redirects
        if (code == HTTP::OK) {
            // success
            bool flushCache = false;
            // delete old cache entry
            auto oldCacheIt = _cache.find(url);
            if (oldCacheIt != _cache.end()) {
                if (oldCacheIt.value()["file"].is_string()) {
                    auto oldPath = _cachedir / fs::u8path(oldCacheIt.value()["file"].get<std::string>());
                    fs::error_code ec;
                    fs::remove(oldPath, ec);
                }
                _cache.erase(oldCacheIt);
                flushCache = true;
            }
            // create new cache entry if etag is provided
            auto etagIt = h.find("etag");
            if (etagIt != h.end()) {
                std::string name;
                int fd = -1;
                for (int i=0; i<5; i++) {
                    name = GetRandomName();
                    auto path = _cachedir / name;
#ifdef _WIN32
                    fd = _wopen(path.c_str(), O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL | O_BINARY, 0600);
#else
                    fd = open(path.c_str(), O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL, 0600);
#endif
                    if (fd>=0) break;
                }
                if (fd >= 0 && write(fd, r.c_str(), r.length()) == (ssize_t)r.length()) {
                    // write success
                    close(fd);
                    time_t t = time(NULL);
                    _cache[url] = {
                        {"file", name},
                        {"etag", etagIt->second},
                        {"timestamp", t}
                    };
                    flushCache = true;
                } else if (fd >= 0) {
                    // write failed
                    auto path = _cachedir / name;
                    printf("Error %d writing cache file \"%s\": %s\n",
                        errno, sanitize_print(path).c_str(), strerror(errno));
                    close(fd);
                    fs::error_code ec;
                    fs::remove(path, ec);
                } else {
                    printf("Could not create cache file!\n");
                }
            }
            if (flushCache) {
                writeFile(_cachefile, _cache.dump());
            }
            // return result
            cb(true, r);
        }
        else if (code == HTTP::NOT_MODIFIED) {
            // use cached file
            std::string cached;
            auto cacheIt = _cache.find(url);
            if (cacheIt.value()["file"].is_string() && readFile(_cachedir / fs::u8path(cacheIt.value()["file"].get<std::string>()), cached)) {
                time_t t = time(NULL);
                _cache[url]["timestamp"] = t;
                cb(true, cached);
            } else {
                printf("Cache destroyed for %s\n", sanitize_print(url).c_str());
                cb(false, "");
            }
        }
        else {
            // error
            cb(false, r);
        }
    }, [cb]() {
        cb(false, "");
    })) {
        fprintf(stderr, "Could not fetch %s\n", url.c_str());
        cb(false, ""); // always fulfill promise
    };
}
