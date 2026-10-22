#include "packmanager.h"
#include <algorithm>
#include "../core/util.h"
#include "../core/fileutil.h"
#include "../core/version.h"
#include "../core/sha256.h"


#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif


PackManager::PackManager(asio::io_service *asio, const fs::path& workdir, const std::list<std::string>& httpDefaultHeaders)
    : HTTPCache(asio, workdir / "pack-cache.json", workdir / "pack-cache", httpDefaultHeaders)
{
    valijson::SchemaParser parser;
    parser.populateSchema(JsonSchemaAdapter(_packsSchemaJson), _packsSchema);
    parser.populateSchema(JsonSchemaAdapter(_versionSchemaJson), _versionSchema);
    _confFile = workdir / "pack-conf.json";
    loadConf();
}

PackManager::~PackManager()
{
}

size_t PackManager::repositoryIndex(const std::string& url) const
{
    for (size_t i = 0; i < _repositories.size(); i++)
        if (_repositories[i].url == url) return i;
    return _repositories.size(); // unknown = lowest priority
}

void PackManager::mergePacks(const std::string& url, const json& packs)
{
    const size_t priority = repositoryIndex(url);
    int n = 0;
    for (const auto& pair: packs.items()) {
        // if the same uid is in multiple lists, the list that comes first wins
        const auto src = _packSources.find(pair.key());
        if (src != _packSources.end() && repositoryIndex(src->second) <= priority)
            continue;
        _packs[pair.key()] = pair.value();
        _packSources[pair.key()] = url;
        n++;
    }
    printf("Added %d packs from %s\n", n, url.c_str());
}

void PackManager::updateNextRepository(std::function<void(bool)> cb, bool modified)
{
    // download only once, in priority order
    const auto next = std::find_if(_repositories.begin(), _repositories.end(),
            [](const Repository& r) { return !r.fetched; });
    if (next == _repositories.end()) {
        cb(modified);
        return;
    }
    const std::string url = next->url;
    GetCached(url, [this, url, cb, modified](bool ok, std::string data)
    {
        // complete handler (success or fail); the list of repositories may have changed in the meantime
        const auto it = std::find_if(_repositories.begin(), _repositories.end(),
                [&url](const Repository& r) { return r.url == url; });
        if (it != _repositories.end()) {
            it->fetched = true; // don't retry even if failed
            if (ok) {
                try {
                    auto j = json::parse(data);
                    valijson::Validator validator;
                    JsonSchemaAdapter jAdapter(j);
                    if (!validator.validate(_packsSchema, jAdapter, nullptr)) {
                        throw std::runtime_error("Json validation failed");
                    }
                    mergePacks(url, j);
                } catch (...) {
                    printf("Invalid json from pack repository %s\n", url.c_str());
                }
            }
        }
        updateNextRepository(cb, ok || modified);
    });
}

void PackManager::updateRepositories(std::function<void(bool)> cb)
{
    updateNextRepository(std::move(cb), false);
}

bool PackManager::checkForUpdate(const std::string& uid, const std::string& version, const std::string& versions_url,
        update_available_callback cb, no_update_available_callback ncb)
{
    if (_tempIgnoredSourceVersion[uid].count(version)) {
        printf("PackManager: Skipping update check for %s %s\n",
                sanitize_print(uid).c_str(), sanitize_print(version).c_str());
        if (ncb) ncb(uid);
        return false;
    }
    printf("PackManager: Checking for update for %s\n", sanitize_print(uid).c_str());
    resolveVersionsUrl(uid, versions_url, [this, uid, version, cb, ncb](const std::string& url) {
        if (url.empty()) {
            if (ncb) ncb(uid);
            return;
        }
        checkForUpdateFrom(uid, version, url, cb, ncb);
    });
    return true; // true = process started
}

void PackManager::resolveVersionsUrl(const std::string& uid, const std::string& versions_url,
        std::function<void(const std::string&)> cb)
{
    // 1. update repositories
    updateRepositories([this, uid, versions_url, cb](bool) {
        // 2. determine which versions_url to use
        std::string url = versions_url;
        auto it = _packs.find(uid);
        if (it != _packs.end() && it.value()["versions_url"].is_string()) {
            url = it.value()["versions_url"].get<std::string>();
            if (!HTTP::is_uri(url)) {
                fprintf(stderr, "WARNING: invalid versions_url in pack!");
                url = versions_url;
            }
        }
        if (url.empty()) {
            printf("Nowhere to check for updates of %s\n", sanitize_print(uid).c_str());
            cb("");
            return;
        }
        // 3. if versions url is not secure, bail out
        std::string proto, host, port, path;
        if (!HTTP::parse_uri(url, proto, host, port, path)) {
            printf("Invalid versions URL: %s\n", sanitize_print(url).c_str());
            cb("");
            return;
        }
        if (proto != "https" && (proto != "http" || host != "localhost" || (port != "" && port != "80"))) {
            printf("Unsupported URL: %s. Please use HTTPS.\n", sanitize_print(url).c_str());
            cb("");
            return;
        }
        // 4. if host is not trusted, ask user
        if (_trustedHosts.find(host) != _trustedHosts.end()) {
            cb(url);
        } else if (_untrustedHosts.find(host) == _untrustedHosts.end()) {
            askConfirmation("Load update information from " + host + "?", [this, url, host, cb](bool res) {
                if (res) {
                    _trustedHosts.insert(host);
                    _conf["trusted_hosts"] = _trustedHosts;
                    saveConf();
                    cb(url);
                } else {
                    _untrustedHosts.insert(host); // don't ask again until restart
                    cb("");
                }
            });
        } else {
            cb("");
        }
    });
}

void PackManager::getPackVersions(const std::string& uid, versions_callback cb)
{
    resolveVersionsUrl(uid, "", [this, cb](const std::string& url) {
        if (url.empty()) {
            cb(false, std::vector<RemoteVersion>{});
            return;
        }
        GetCached(url, [this, url, cb](bool ok, std::string data) {
            std::vector<RemoteVersion> out;
            if (ok) {
                try {
                    auto j = json::parse(data);
                    valijson::Validator validator;
                    JsonSchemaAdapter jAdapter(j);
                    if (!validator.validate(_versionSchema, jAdapter, nullptr))
                        throw std::runtime_error("Json validation failed");
                    for (auto& section : j["versions"]) {
                        if (!section["download_url"].is_string())
                            continue; // "null" = no download for this version
                        RemoteVersion v;
                        v.version = section["package_version"].get<std::string>();
                        v.url = section["download_url"].get<std::string>();
                        v.sha256 = section["sha256"].get<std::string>(); // required by schema if url is set
                        if (v.version.empty() || v.url.empty() || v.sha256.empty())
                            continue;
                        for (const auto& line : section["changelog"])
                            if (line.is_string()) v.changelog.push_back(line.get<std::string>());
                        out.push_back(std::move(v));
                    }
                } catch (...) {
                    printf("Invalid json versions %s\n", sanitize_print(url).c_str());
                    ok = false;
                    out.clear();
                }
            }
            cb(ok, out);
        });
    });
}

void PackManager::testRepository(const std::string& url, std::function<void(bool)> cb)
{
    GetCached(url, [this, cb](bool ok, std::string data) {
        bool valid = false;
        if (ok) {
            try {
                auto j = json::parse(data);
                valijson::Validator validator;
                JsonSchemaAdapter jAdapter(j);
                valid = validator.validate(_packsSchema, jAdapter, nullptr);
            } catch (...) {}
        }
        cb(valid);
    });
}

void PackManager::getAvailablePacks(std::function<void(const json&)> cb)
{
    // update repositories
    updateRepositories([this, cb](bool) {
        cb(_packs);
    });
}

void PackManager::checkForUpdateFrom(const std::string& uid, const std::string& version, const std::string url,
        update_available_callback cb, no_update_available_callback ncb)
{
    GetCached(url, [this, uid, version, url, cb, ncb](bool ok, std::string data) {
        bool hasUpdate = false;
        if (ok) {
            try {
                auto j = json::parse(data);
                valijson::Validator validator;
                JsonSchemaAdapter jAdapter(j);
                if (!validator.validate(_versionSchema, jAdapter, nullptr)) {
                    throw std::runtime_error("Json validation failed");
                }

                Version current = Version(version);

                for (auto& section: j["versions"]) {
                    if (!section["download_url"].is_string())
                        continue; // skip "null"
                    std::string v = section["package_version"].get<std::string>();
                    if (v.empty())
                        continue; // skip bad version names
                    auto ignoreIt = _ignoredSHA256.find(uid);
                    if (ignoreIt != _ignoredSHA256.end() &&
                            ignoreIt->second.count(section["sha256"].get<std::string>())) {
                        break;
                    }
                    Version check = Version(v);
                    if (check > current) {
                        hasUpdate = true;
                        onUpdateAvailable.emit(this,
                                uid, v,
                                section["download_url"].get<std::string>(),
                                section["sha256"].get<std::string>());
                        if (cb) {
                            cb(uid, v,
                               section["download_url"].get<std::string>(),
                               section["sha256"].get<std::string>());
                        }
                    }
                    break;
                }
                if (!hasUpdate) printf("Already up to date!\n");
            } catch (...) {
                printf("Invalid json versions %s\n", sanitize_print(url).c_str());
            }
        } else {
            printf("Could not fetch %s\n", sanitize_print(url).c_str());
        }
        if (!hasUpdate && ncb) ncb(uid);
    });
}

void PackManager::askConfirmation(const std::string& message, std::function<void(bool)> cb)
{
    if (_confirmationHandler)
        _confirmationHandler(message, cb);
    else
        cb(false);
}

void PackManager::ignoreUpdateSHA256(const std::string& uid, const std::string& sha256)
{
    _ignoredSHA256[uid].insert(sha256);
    _conf["ignored_sha256"] = _ignoredSHA256;
    saveConf();
}

void PackManager::tempIgnoreSourceVersion(const std::string& uid, const std::string& version)
{
    _tempIgnoredSourceVersion[uid].insert(version);
}

void PackManager::GetFile(const std::string& url, const fs::path& dest,
        std::function<void(bool,const std::string&)> cb,
        std::function<void(int,int)> progress, int followRedirect)
{
#ifdef _WIN32
    FILE* f = _wfopen(dest.c_str(), L"wb");
#else
    FILE* f = fopen(dest.c_str(), "wb");
#endif
    if (!f) cb(false, strerror(errno));
    const bool started = HTTP::GetAsync(*_asio, url, _httpDefaultHeaders,
            [=](const int code, const std::string& response, const HTTP::Headers&) {
        fclose(f);
        if (code == HTTP::REDIRECT && followRedirect>0) {
            GetFile(response, dest, cb, progress, followRedirect-1);
        } else if (code == HTTP::REDIRECT) {
            cb(false, "Too many redirects");
        } else {
            cb(code==HTTP::OK, response);
        }
    }, [=]() {
        fclose(f);
        cb(false, "");
    }, f, progress);

    if (!started) {
        fclose(f);
        cb(false, "Could not start download");
    }
}

void PackManager::downloadUpdate(const std::string& url, const fs::path& install_dir,
        const std::string& validate_uid, const std::string& validate_version, const std::string& validate_sha256)
{
    fs::path path;
    int fd = -1;
    for (int i=0; i<5; i++) {
        std::string name = GetRandomName(".zip");
        path = _cacheDir / name;
#ifdef _WIN32
        fd = _wopen(path.c_str(), O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL, 0600);
#else
        fd = open(path.c_str(), O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL, 0600);
#endif
        if (fd>=0)
            break;
    }
    if (fd < 0) {
        onUpdateFailed.emit(this, url, "Could not create temp file!");
        return;
    }
    close(fd);
    GetFile(url, path, [=](bool ok, const std::string& msg){
        if (ok) {
            if (!validate_sha256.empty()) {
                auto hash = SHA256_File(path);
                if (hash.empty()) {
                    fs::error_code ec;
                    fs::remove(path, ec);
                    onUpdateFailed.emit(this, url, "Could not calculate hash");
                    return;
                }
                if (strcasecmp(hash.c_str(), validate_sha256.c_str()) != 0) {
                    fs::error_code ec;
                    fs::remove(path, ec);
                    onUpdateFailed.emit(this, url, "SHA256 mismatch");
                    return;
                }
            }

            Pack *pack = new Pack(path);
            if (!pack->isValid()) {
                delete pack;
                fs::error_code ec;
                fs::remove(path, ec);
                onUpdateFailed.emit(this, url, "Not a valid pack");
                return;
            }
            std::string newVersion = pack->getVersion();
            std::string newUid = pack->getUID();
            auto newPath = install_dir / sanitize_filename(newUid + "_" + newVersion + ".zip");
            delete pack;
            pack = nullptr;

            if (!validate_version.empty() && newVersion != validate_version) {
                std::string err = "Version mismatch: " + newVersion + " != " + validate_version;
                fs::error_code ec;
                fs::remove(path, ec);
                onUpdateFailed.emit(this, url, err);
                return;
            }
            auto install = [this, url, path, newPath, newUid]() {
                fs::error_code ec;
                fs::rename(path, newPath, ec);
                if (ec) {
                    fs::error_code ignore;
                    fs::remove(path, ignore);
                    onUpdateFailed.emit(this, url, "Error moving: " + ec.message());
                    return;
                }
                onUpdateComplete.emit(this, url, newPath, newUid);
            };
            if (!validate_uid.empty() && newUid != validate_uid) {
                askConfirmation("UID mismatch, install anyway?", [=](bool res) {
                    if (res) install();
                    else {
                        std::string err = "UID mismatch: " + newUid + " != " + validate_uid;
                        fs::error_code ec;
                        fs::remove(path, ec);
                        onUpdateFailed.emit(this, url, err);
                    }
                });
                return;
            }
            install();
        } else {
            onUpdateFailed.emit(this, url, msg);
        }
    }, [this, url](int transferred, int total) {
        onUpdateProgress.emit(this, url, transferred, total);
    });
}

void PackManager::loadConf()
{
    std::string s;
    bool updated = false;
    if (readFile(_confFile, s)) {
        try {
            _conf = json::parse(s);
            if (_conf["trusted_hosts"].is_array()) {
                for (const auto& host: _conf["trusted_hosts"]) {
                    if (host.is_string()) _trustedHosts.insert(host.get<std::string>());
                }
            }
            if (_conf["ignored_sha256"].is_object()) {
                for (const auto& pair: _conf["ignored_sha256"].items()) {
                    if (pair.value().is_array()) {
                        for (const auto& sha256 : pair.value()) {
                            _ignoredSHA256[pair.key()].insert(sha256.get<std::string>());
                        }
                    }
                }
            }
        } catch (json::exception& ex) {
            printf("PackManager config is broken: %s\n", ex.what());
            _conf = json::object();
        }
    }
    if (_trustedHosts.empty()) {
        // fill with defaults
        for (const auto& host: {
                "github.com",
                "www.github.com",
                "raw.githubusercontent.com" 
        }) {
            _trustedHosts.insert(host);
        }
        _conf["trusted_hosts"] = _trustedHosts;
        updated = true;
    }
    if (updated) {
        saveConf();
    }
}

void PackManager::saveConf()
{
    writeFile(_confFile, _conf.dump(4) + "\n");
}
