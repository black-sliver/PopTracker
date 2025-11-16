#pragma once

#include <string>
#include <nlohmann/json.hpp>


namespace gh::api {
    class ReleaseAsset {
        using json = nlohmann::json;
    public:
        void update(const json& j);

        const std::string& browserDownloadUrl() const
        {
            return _browserDownloadUrl;
        }

        bool nameEndsWithCI(const std::string& s) const;

    private:
        std::string _name;
        std::string _browserDownloadUrl;
    };

    void from_json(const nlohmann::json& j, ReleaseAsset& r);
} // gh::api
