#include "assets.h"
#include "fileutil.h"
#include <algorithm>

std::vector<std::string> Assets::_searchPaths;

void Assets::addSearchPath(const std::string& path)
{
    if (std::find(_searchPaths.begin(), _searchPaths.end(), path) != _searchPaths.end()) return;
    _searchPaths.push_back(path);
}

std::string Assets::Find(const std::string& name)
{
    for (auto& searchPath : _searchPaths) {
        auto filename = os_pathcat(searchPath, name);
        if (fileExists(filename)) return filename;
    }
    return os_pathcat("assets", name); // fall-back to CWD
}