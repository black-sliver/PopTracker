#ifndef _CORE_MAP_H
#define _CORE_MAP_H

#include <nlohmann/json.hpp>
#include <string>

class Map final {
public:
    static Map FromJSON(nlohmann::json& j);
    static Map FromJSON(nlohmann::json&& j);
    static Map FromJSONString(const std::string& json);

protected:
    int _locationSize=16; // TODO: default?
    int _locationBorderThickness=1; // TODO: default?
    std::string _img;

public:
    int getLocationSize() const { return _locationSize; }
    int getLocationBorderTickness() const { return _locationBorderThickness; }
    const std::string& getImage() const { return _img; }    
};

#endif // _CORE_MAP_H
