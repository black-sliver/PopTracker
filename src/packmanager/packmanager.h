#ifndef _PACKMANAGER_PACKMANAGER_H
#define _PACKMANAGER_PACKMANAGER_H


#include <nlohmann/json.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>
#include <map>
#include <string>
#include <functional>
#include <vector>
#include "../http/http.h"
#include "../http/httpcache.h"
#include "../core/fs.h"
#include "../core/signal.h"
#include "../core/pack.h"


class PackManager final : HTTPCache {
    typedef nlohmann::json json;
    typedef valijson::adapters::NlohmannJsonAdapter JsonSchemaAdapter;
public:
    typedef std::function<void(const std::string&, const std::string&, const std::string&, const std::string&)> update_available_callback;
    typedef std::function<void(const std::string&)> no_update_available_callback;
    typedef std::function<void(std::string, std::function<void(bool)>)> confirmation_callback;

    PackManager(asio::io_service *asio, const fs::path& workdir, const std::list<std::string>& httpDefaultHeaders={});
    PackManager(const PackManager&) = delete;
    virtual ~PackManager();

    struct Repository {
        std::string name;
        std::string url;
        bool fetched = false;
    };

    /// only allow https:// for user and http://localhost/ for testing
    static bool isValidRepositoryUrl(const std::string& url) {
        return strncasecmp(url.c_str(), "https://", 8) == 0 ||
                strncasecmp(url.c_str(), "http://localhost/", 17) == 0;
    }

    /// lists added earlier have priority over lists added later.
    bool addRepository(const std::string& url, const std::string& name = "") {
        if (!isValidRepositoryUrl(url))
            return false;
        for (auto& repository : _repositories) {
            if (repository.url == url) {
                if (!name.empty()) repository.name = name;
                return true;
            }
        }
        // schedule update (fetched = false)
        _repositories.push_back(Repository{name.empty() ? url : name, url, false});
        return true;
    }

    bool checkForUpdate(const Pack* pack, update_available_callback cb=nullptr, no_update_available_callback ncb=nullptr) {
        if (!pack) return false;
        return checkForUpdate(pack->getUID(), pack->getVersion(), pack->getVersionsURL(), cb, ncb);
    }

    bool checkForUpdate(const std::string& uid, const std::string& version, const std::string& versions_url,
                        update_available_callback cb=nullptr, no_update_available_callback ncb=nullptr);
    
    void setConfirmationHandler(confirmation_callback f) {
        _confirmationHandler = f;
    }

    void getAvailablePacks(std::function<void(const json&)> cb);
    void refreshAvailablePacks(std::function<void(const json&)> cb) {
        for (auto& repository : _repositories) repository.fetched = false;
        _packs = json::object();
        _packSources.clear();
        getAvailablePacks(std::move(cb));
    }
    std::string getAvailablePackName(const std::string& uid) const {
        const auto it = _packs.find(uid);
        return it != _packs.end() ? it.value().value("name", uid) : uid;
    }
    /// Name of the pack list the pack (by uid) was taken from, empty if unknown.
    std::string getAvailablePackListName(const std::string& uid) const {
        const auto src = _packSources.find(uid);
        if (src == _packSources.end()) return "";
        for (const auto& repository : _repositories)
            if (repository.url == src->second) return repository.name;
        return "";
    }
    void ignoreUpdateSHA256(const std::string& uid, const std::string& sha256);
    void tempIgnoreSourceVersion(const std::string& uid, const std::string& version);
    void downloadUpdate(const std::string& url, const fs::path& install_dir,
            const std::string& validate_uid, const std::string& validate_version, const std::string& validate_sha256);

    Signal<const std::string&, const std::string&, const std::string&, const std::string&> onUpdateAvailable;
    Signal<const std::string&, int, int> onUpdateProgress;
    Signal<const std::string&, const fs::path&, const std::string&> onUpdateComplete;
    Signal<const std::string&, const std::string&> onUpdateFailed;

    struct RemoteVersion {
    std::string version;
    std::string url;
    std::string sha256;
    std::vector<std::string> changelog;
    };
    typedef std::function<void(bool, const std::vector<RemoteVersion>&)> versions_callback;

    bool hasRepositories() const { return !_repositories.empty(); }
    void clearRepositories() { _repositories.clear(); _packs = json::object(); _packSources.clear(); }
    /// Fetches and validates a pack list; cb(true) if it is a valid pack list.
    void testRepository(const std::string& url, std::function<void(bool)> cb);
    /// Lists all downloadable versions of a pack from its versions.json (newest first as in the file).
    void getPackVersions(const std::string& uid, versions_callback cb);


private:
    void updateNextRepository(std::function<void(bool)> cb, bool modified);
    size_t repositoryIndex(const std::string& url) const;
    void mergePacks(const std::string& url, const json& packs);
    void updateRepositories(std::function<void(bool)> cb);
    void checkForUpdateFrom(const std::string& uid, const std::string& version, const std::string url, update_available_callback cb, no_update_available_callback ncb);
    void askConfirmation(const std::string& message, std::function<void(bool)> cb);
    void loadConf();
    void saveConf();
    void GetFile(const std::string& url, const fs::path& dest,
            std::function<void(bool,const std::string&)> cb,
            std::function<void(int,int)> progress, int followRedirect=3);
    void resolveVersionsUrl(const std::string& uid, const std::string& versions_url,
                        std::function<void(const std::string&)> cb); // cb("") on failure/denied


    fs::path _confFile;
    std::vector<Repository> _repositories;
    json _packs = json::object();
    std::map<std::string, std::string> _packSources; // pack uid -> url of the list the entry in _packs came from
    json _conf = json::object();
    std::set<std::string> _trustedHosts; // hosts allowed to connect to for versions json
    std::set<std::string> _untrustedHosts; // user replied "no" when being asked to connect
    std::map<std::string, std::set<std::string>> _skippedVersions;
    confirmation_callback _confirmationHandler;
    std::map<std::string, std::set<std::string>> _ignoredSHA256;
    std::map<std::string, std::set<std::string>> _tempIgnoredSourceVersion;

    valijson::Schema _packsSchema;
    valijson::Schema _versionSchema;

    const json _packsSchemaJson = R"({
        "type": "object",
        "additionalProperties": {
            "type": "object",
            "properties": {
                "name": { "type": "string" },
                "author": { "type": "string" },
                "platform": { "type": "string" },
                "homepage": { "type": "string", "format": "uri" },
                "versions_url": { "type": "string", "format": "uri" },
                "description": { "type": "string" },
                "icon_url": { "type": "string", "format": "uri" }
            },
            "required": [
                "name",
                "author",
                "platform",
                "homepage",
                "versions_url",
                "description"
            ]
        }
    })"_json;
    const json _versionSchemaJson = R"({
        "type": "object",
        "properties": {
            "versions" : {
                "type": "array",
                "items": {
                    "type": "object",
                    "properties": {
                        "package_version": {
                            "type": "string"
                        },
                        "download_url": {
                            "type": ["string", "null"],
                            "format": "uri"
                        },
                        "sha256": {
                            "description": "SHA256 of the download as hex chars without spaces",
                            "type": "string",
                            "minLength": 64,
                            "maxLength": 64
                        },
                        "changelog": {
                            "type": "array",
                            "items": {
                                "type": "string"
                            }
                        }
                    },
                    "required": [
                        "package_version",
                        "download_url",
                        "changelog"
                    ],
                    "oneOf": [
                        {
                            "properties": {
                                "download_url": {
                                    "type": "null"
                                }
                            }
                        },
                        {
                            "required": ["sha256"]
                        }
                    ]
                }
            }
        },
        "required": [ "versions" ]
    })"_json;
};

#endif /* _PACKMANAGER_PACKMANAGER_H */
