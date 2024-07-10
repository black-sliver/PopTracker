#include "map.h"
#include "jsonutil.h"


using nlohmann::json;


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
    map._locationShape = ::Location::MapLocation::ShapeFromString(to_string(j["location_shape"], ""));
    map._img = to_string(j["img"], "");
    return map;
}
