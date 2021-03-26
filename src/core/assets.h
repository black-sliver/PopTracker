#ifndef _CORE_ASSETS_H
#define _CORE_ASSETS_H

#include <string>
#include <vector>

class Assets final {
public:
    static std::string Find(const std::string& name);
    static void addSearchPath(const std::string& path);
private:
    static void initialize();
    static std::vector<std::string> _searchPaths;
};

static std::string asset(std::string name) { return Assets::Find(name); }

#endif /* _CORE_ASSETS_H */

