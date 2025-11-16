#include "release.hpp"
#include <nlohmann/json.hpp>


namespace gh::api {

void from_json(const nlohmann::json& j, Release& r)
{
    r.update(j);
}

void Release::update(const json &j)
{
    j.at("tag_name").get_to(_tagName);
    j.at("prerelease").get_to(_prerelease);
    j.at("html_url").get_to(_htmlUrl);
    j.at("assets").get_to(_assets);
    j.at("body").get_to(_body);
}

} // gh::api
