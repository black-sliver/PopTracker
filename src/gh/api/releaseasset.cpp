#include "releaseasset.hpp"
#include <cstring>
#include <nlohmann/json.hpp>


namespace gh::api {

void from_json(const nlohmann::json& j, ReleaseAsset& a)
{
    a.update(j);
}

void ReleaseAsset::update(const json &j)
{
    j.at("name").get_to(_name);
    j.at("browser_download_url").get_to(_browserDownloadUrl);
}

bool ReleaseAsset::nameEndsWithCI(const std::string &s) const
{
    if (_name.length() < s.length())
        return false;
    return strcasecmp(_name.c_str() + _name.length() - s.length(), s.c_str()) == 0;
}

} // gh::api
