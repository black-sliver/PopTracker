#ifndef _CORE_MAP_H
#define _CORE_MAP_H

#include <nlohmann/json.hpp>
#include <string>
#include "location.h"


class Map final {
public:
    typedef ::Location::MapLocation::Shape LocationShape;

    static Map FromJSON(nlohmann::json& j);
    static Map FromJSON(nlohmann::json&& j);
    static Map FromJSONString(const std::string& json);

protected:
    int _locationSize=65;
    int _locationBorderThickness=8;
    LocationShape _locationShape = LocationShape::UNSPECIFIED;
    std::string _img;

public:
    int getLocationSize() const { return _locationSize; }
    int getLocationBorderThickness() const { return _locationBorderThickness; }
    LocationShape getLocationShape() const
    {
        return _locationShape;
    }
    const std::string& getImage() const { return _img; } 
};

#endif // _CORE_MAP_H
