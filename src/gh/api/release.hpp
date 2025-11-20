#pragma once

#include <vector>
#include <nlohmann/json.hpp>
#include "releaseasset.hpp"
#include "../../core/fs.h"


namespace gh::api {
    class Release final {
        using json = nlohmann::json;
    public:
        void update(const json& j);

        const std::string& tagName() const
        {
            return _tagName;
        }

        bool prerelease() const
        {
            return _prerelease;
        }

        const std::string& htmlUrl() const
        {
            return _htmlUrl;
        }

        const std::vector<ReleaseAsset>& assets() const
        {
            return _assets;
        }

        const std::string& body() const
        {
            return _body;
        }

        bool hasAssetUrl(const std::string& url) const
        {
            return std::any_of(_assets.begin(), _assets.end(), [&url](const ReleaseAsset& asset) {
                return asset.browserDownloadUrl() == url;
            });
        }

        bool bodyContains(const std::string& s) const
        {
            return _body.find(s) != std::string::npos;
        }

        bool bodyContains(const fs::path& path) const
        {
            return bodyContains(path.u8string());
        }

    private:
        std::string _tagName;
        bool _prerelease = false;
        std::string _htmlUrl;
        std::vector<ReleaseAsset> _assets;
        std::string _body;
    };

    void from_json(const nlohmann::json& j, Release& r);
} // gh::api
