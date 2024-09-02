#include "packmanager.h"
#include "../core/util.h"
#include "../core/fileutil.h"
#include "../core/version.h"
#include "../core/sha256.h"


#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif


PackManager::PackManager(asio::io_service *asio, const std::string& workdir, const std::list<std::string>& httpDefaultHeaders)
    : HTTPCache(asio, os_pathcat(workdir, "pack-cache.json"), os_pathcat(workdir, "pack-cache"), httpDefaultHeaders)
{
    valijson::SchemaParser parser;
    parser.populateSchema(JsonSchemaAdapter(_packsSchemaJson), _packsSchema);
    parser.populateSchema(JsonSchemaAdapter(_versionSchemaJson), _versionSchema);
    _confFile = os_pathcat(workdir, "pack-conf.json");
    loadConf();
}

PackManager::~PackManager()
{
}

void PackManager::updateRepository(const std::string& url, std::function<void(bool)> cb, bool alreadyModified)
{
    GetCached(url, [this, url, cb, alreadyModified](bool ok, std::string data)
    {
        // complete handler (success or fail)
        auto last = _repositories.find(url);
        if (last != _repositories.end()) {
            last->second = true; // don't retry even if failed
            if (ok) {
                try {
                    auto j = json::parse(data);
                    valijson::Validator validator;
                    JsonSchemaAdapter jAdapter(j);
                    if (!validator.validate(_packsSchema, jAdapter, nullptr)) {
                        throw std::runtime_error("Json validation failed");
                    }
                    int n = 0;
                    for (auto& pair: j.items()) {
                        _packs[pair.key()] = pair.value();
                        n++;
                    }
                    printf("Added %d packs from %s\n", n, url.c_str());
                } catch (...) {
                    printf("Invalid json from pack repository %s\n", url.c_str());
                }
            }
            last++;
        }
        while (last != _repositories.end() && last->second) // download only once
            last++;
        if (last == _repositories.end()) {
            cb(ok || alreadyModified); // all done
        } else {
            std::string nextURL = last->first;
            updateRepository(nextURL, cb, ok || alreadyModified);
        }
    });
}

void PackManager::updateRepositories(std::function<void(bool)> cb)
{
    auto first = _repositories.begin();
    while (first != _repositories.end() && first->second) // download only once
        first++;
    if (first != _repositories.end()) {
        auto firstURL = first->first;
        updateRepository(firstURL, cb);
    } else {
        cb(false);
    }
}

bool PackManager::checkForUpdate(const std::string& uid, const std::string& version, const std::string& versions_url,
        update_available_callback cb, no_update_available_callback ncb)
{
    if (_tempIgnoredSourceVersion[uid].count(version)) {
        printf("PackaManager: Skipping update check for %s %s\n",
                sanitize_print(uid).c_str(), sanitize_print(version).c_str());
        if (ncb) ncb(uid);
        return false;
    }
    printf("PackManager: Checking for update for %s\n", sanitize_print(uid).c_str());
    // 1. update repositories
    updateRepositories([this, uid, version, versions_url, cb, ncb](bool) {
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
            printf("Nowhere to check for updates of %s\n", uid.c_str());
            if (ncb) ncb(uid);
            return;
        }
        // 3. if versions url is not secure, bail out
        std::string proto, host, port, path;
        if (!HTTP::parse_uri(url, proto, host, port, path)) {
            printf("Invalid versions URL: %s\n", sanitize_print(url).c_str());
            if (ncb) ncb(uid);
            return;
        }
        if (proto != "https" && (proto != "http" || host != "localhost" || (port != "" && port != "80"))) {
            printf("Unsupported URL: %s. Please use HTTPS.\n",  sanitize_print(url).c_str());
            if (ncb) ncb(uid);
            return;
        }
        // 4. if host is not trusted, ask user
        if (_trustedHosts.find(host) != _trustedHosts.end()) {
            // 5. fetch and check versions.json
            checkForUpdateFrom(uid, version, url, cb, ncb);
        } else if (_untrustedHosts.find(host) == _untrustedHosts.end()) {
            askConfirmation("Load update information from " + host + "?", [this, uid, version, url, host, cb, ncb](bool res){
                if (res) {
                    _trustedHosts.insert(host);
                    _conf["trusted_hosts"] = _trustedHosts;
                    saveConf();
                    // 5. fetch and check versions.json
                    checkForUpdateFrom(uid, version, url, cb, ncb);
                }
                else {
                    // don't ask again until restart
                    _untrustedHosts.insert(host);
                    if (ncb) ncb(uid);
                }
            });
        }
    });
    return true; // true = process started
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

void PackManager::GetFile(const std::string& url, const std::string& dest,
        std::function<void(bool,const std::string&)> cb,
        std::function<void(int,int)> progress, int followRedirect)
{
    FILE* f = fopen(dest.c_str(), "wb");
    if (!f) cb(false, strerror(errno));
    HTTP::GetAsync(*_asio, url, _httpDefaultHeaders, [=](int code, const std::string& response, const HTTP::Headers&){
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
}

void PackManager::downloadUpdate(const std::string& url, const std::string& install_dir,
        const std::string& validate_uid, const std::string& validate_version, const std::string& validate_sha256)
{
    std::string path;
    int fd = -1;
    for (int i=0; i<5; i++) {
        std::string name = GetRandomName(".zip");
        path = os_pathcat(_cachedir, name);
        fd = open(path.c_str(), O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL, 0600);
        if (fd>=0) break;
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
                    unlink(path.c_str());
                    onUpdateFailed.emit(this, url, "Could not calculate hash");
                    return;
                }
                if (strcasecmp(hash.c_str(), validate_sha256.c_str()) != 0) {
                    unlink(path.c_str());
                    onUpdateFailed.emit(this, url, "SHA256 mismatch");
                    return;
                }
            }

            Pack *pack = new Pack(path);
            if (!pack->isValid()) {
                delete pack;
                unlink(path.c_str());
                onUpdateFailed.emit(this, url, "Not a valid pack");
                return;
            }
            std::string newVersion = pack->getVersion();
            std::string newUid = pack->getUID();
            std::string newPath = os_pathcat(install_dir,
                    sanitize_filename(newUid+"_"+newVersion+".zip"));
            delete pack;
            pack = nullptr;

            if (!validate_version.empty() && newVersion != validate_version) {
                std::string err = "Version mismatch: " + newVersion + " != " + validate_version;
                unlink(path.c_str());
                onUpdateFailed.emit(this, url, err);
                return;
            }
            auto install = [this, url, path, newPath, newUid]() {
                if (rename(path.c_str(), newPath.c_str()) != 0) {
                    unlink(path.c_str());
                    onUpdateFailed.emit(this, url, std::string("Error moving: ") + strerror(errno));
                    return;
                }
                onUpdateComplete.emit(this, url, newPath, newUid);
            };
            if (!validate_uid.empty() && newUid != validate_uid) {
                askConfirmation("UID mismatch, install anyway?", [=](bool res) {
                    if (res) install();
                    else {
                        std::string err = "UID mismatch: " + newUid + " != " + validate_uid;
                        unlink(path.c_str());
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
