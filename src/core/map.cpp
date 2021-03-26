#include "map.h"
using nlohmann::json;

static std::string to_string(const json& j, const std::string& dflt)
{
    return (j.type() == json::value_t::string) ? j.get<std::string>() : dflt;
}
static int to_int(const json& j, int dflt)
{
    // TODO: float?
    return (j.type() == json::value_t::number_integer) ? j.get<int>() : 
           (j.type() == json::value_t::number_unsigned) ? (int)j.get<unsigned>() : dflt;
}

Map Map::FromJSONString(const std::string& j)
{
    return FromJSON(json::parse(j, nullptr, true, true));
}
Map Map::FromJSON(json&& j)
{
    auto tmp = j;
    return FromJSON(tmp);
}
Map Map::FromJSON(json& j)
{
    Map map;
    map._locationSize = to_int(j["location_size"], map._locationSize);
    map._locationBorderThickness = to_int(j["location_border_thickness"], map._locationBorderThickness);
    map._img = to_string(j["img"], "");
    return map;
}
