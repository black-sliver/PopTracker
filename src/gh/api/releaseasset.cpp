#include "releaseasset.hpp"
#include <chrono>
#include <cstring>
#include <nlohmann/json.hpp>
#include "../util/date.h"


namespace gh::api {

void from_json(const nlohmann::json& j, ReleaseAsset& a)
{
    a.update(j);
}

static uint64_t rfc3339UTCToSeconds(const std::string& s)
{
    date::sys_time<std::chrono::seconds> tp;
    std::istringstream stream{s};
    stream.exceptions(std::ios::failbit); // throw exception on parse error
    stream >> date::parse("%FT%TZ", tp);
    return tp.time_since_epoch().count();
}

void ReleaseAsset::update(const json &j)
{
    j.at("name").get_to(_name);
    j.at("browser_download_url").get_to(_browserDownloadUrl);
    _updatedAt = rfc3339UTCToSeconds(j.at("updated_at").get<std::string>());
}

bool ReleaseAsset::nameEndsWithCI(const std::string &s) const
{
    if (_name.length() < s.length())
        return false;
    return strcasecmp(_name.c_str() + _name.length() - s.length(), s.c_str()) == 0;
}

} // gh::api
