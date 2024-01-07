#ifndef _CORE_PACK_H
#define _CORE_PACK_H

#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <nlohmann/json.hpp>
#include "zip.h"
#include "version.h"


class Pack final {
public:
    struct VariantInfo {
        std::string variant;
        std::string name;
    };
    struct Info {
        std::string path;
        std::string uid;
        std::string version;
        std::string platform;
        std::string gameName;
        std::string packName;
        Version minPoptrackerVersion;
        std::vector<VariantInfo> variants;
    };
    
    Pack(const std::string& path);
    virtual ~Pack();

    bool isValid() const
    {
        return !_uid.empty();
        // TODO: also check if init.lua exists?
    }

    void setVariant(const std::string& variant);
    const std::string& getPath() const { return _path; }
    const std::string& getUID() const { return _uid; }
    const std::string& getVariant() const { return _variant; }
    const std::string& getName() const { return _name; }
    const std::string& getGameName() const { return _gameName; }
    const std::string& getVariantName() const { return _variantName; }
    const std::string& getVersionsURL() const { return _versionsURL; }
    std::string getVariantTitle() const { return _variantName.empty() ? _gameName : (_gameName + "-" + _variantName); }
    const Version& getMinPopTrackerVersion() const { return _minPopTrackerVersion; }
    const Version& getTargetPopTrackerVersion() const { return _targetPopTrackerVersion; }
    const nlohmann::json& getSettings() const { return _settings; }

    Info getInfo() const;
    
    bool hasFile(const std::string& file) const;
    bool ReadFile(const std::string& file, std::string& out) const;
    
    bool variantHasFlag(const std::string& flag) const;
    std::set<std::string> getVariantFlags() const;
    std::string getPlatform() const;
    std::string getVersion() const;
    bool hasFilesChanged() const;
    std::string getSHA256() const;
    
    static std::vector<Info> ListAvailable();
    static Info Find(const std::string& uid, const std::string& version="", const std::string& sha256="");
    static void addSearchPath(const std::string& path);
    static bool isInSearchPath(const std::string& path);
    static const std::vector<std::string>& getSearchPaths();

    static void addOverrideSearchPath(const std::string& path);

private:
    class Override {
    public:
        Override(const std::string& path);
        virtual ~Override();

        bool ReadFile(const std::string& file, std::string& out) const;
        bool hasFilesChanged(std::chrono::system_clock::time_point since) const;
        const std::string& getPath() const { return _path; }

    private:
        std::string _path;
    };

    Zip* _zip;
    std::string _path;
    std::string _variant;
    std::string _uid;
    std::string _name;
    std::string _gameName;
    std::string _variantName;
    std::string _versionsURL;
    Version _minPopTrackerVersion;
    Version _targetPopTrackerVersion;
    nlohmann::json _manifest;
    nlohmann::json _settings;
    Override* _override;

    std::chrono::system_clock::time_point _loaded;

    static std::vector<std::string> _searchPaths;
    static std::vector<std::string> _overrideSearchPaths;
};

#endif // _CORE_PACK_H
