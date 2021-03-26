#include "baseitem.h"
#include "jsonutil.h"
using nlohmann::json;


std::string BaseItem::getCodesString() const {
    // this returns a human-readable list, for debugging
    std::string s;
    for (const auto& code: _codes) {
        if (!s.empty()) s += ", ";
        s += code;
    }
    return s;
}


