#ifndef _CORE_PACK_H
#define _CORE_PACK_H

#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <nlohmann/json.hpp>
#include "zip.h"

class Pack {
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
        std::vector<VariantInfo> variants;
    };
    
    Pack(const std::string& path);
    virtual ~Pack();
    bool isValid() const { return _manifest.type() == nlohmann::json::value_t::object; } // TODO: also check if init.lua exists?
    
    void setVariant(const std::string& variant);
    const std::string& getPath() const { return _path; }
    const std::string& getUID() const { return _uid; }
    const std::string& getVariant() const { return _variant; }
    const std::string& getName() const { return _name; }
    const std::string& getGameName() const { return _gameName; }
    const std::string& getVariantName() const { return _variantName; }
    std::string getVariantTitle() const { return _variantName.empty() ? _gameName : (_gameName + "-" + _variantName); }
    
    Info getInfo() const;
    
    bool hasFile(const std::string& file) const;
    bool ReadFile(const std::string& file, std::string& out) const;
    
    bool variantHasFlag(const std::string& flag) const;
    std::set<std::string> getVariantFlags() const;
    std::string getPlatform() const;
    std::string getVersion() const;
    bool hasFilesChanged() const;
    
    static std::vector<Info> ListAvailable();
    static Info Find(std::string uid, std::string version="");
    static void addSearchPath(const std::string& path);
    static bool isInSearchPath(const std::string& path);

protected:
    Zip* _zip;
    std::string _path;
    std::string _variant;
    std::string _uid;
    std::string _name;
    std::string _gameName;
    std::string _variantName;
    nlohmann::json _manifest;

    std::chrono::system_clock::time_point _loaded;

    static std::vector<std::string> _searchPaths;
};

#endif // _CORE_PACK_H
