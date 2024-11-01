#ifndef _CORE_ASSETS_H
#define _CORE_ASSETS_H

#include <string>
#include <vector>
#include "fs.h"

class Assets final {
public:
    static fs::path Find(const std::string& name);
    static void addSearchPath(const fs::path& path);
private:
    static void initialize();
    static std::vector<fs::path> _searchPaths;
};

static fs::path asset(const std::string& name) { return Assets::Find(name); }

#endif /* _CORE_ASSETS_H */

