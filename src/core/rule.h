#pragma once

#include <cstdio>
#include <nlohmann/json.hpp>
#include "jsonutil.h"
#include "util.h"


static bool parseRule(const nlohmann::json& v, std::list<std::string>& rule,
                      const char* nodeType, const char* ruleType, const std::string& name)
{
    if (v.is_string()) {
        // string with individual codes separated by comma
        commasplit(v, rule);
    }
    else if (v.is_array()) {
        // we also allow rules to be arrays of strings instead
        for (const auto& part: v) {
            if (!part.is_string()) {
                fprintf(stderr, "%s: bad %s rule in \"%s\"\n",
                    nodeType, ruleType, sanitize_print(name).c_str());
                continue;
            }
            rule.push_back(part);
        }
    }
    else {
        fprintf(stderr, "%s: bad %s rule in \"%s\"\n",
            nodeType, ruleType, sanitize_print(name).c_str());
        return false;;
    }
    return true;
}
