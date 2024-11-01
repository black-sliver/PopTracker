#include "assets.h"
#include "fileutil.h"
#include <algorithm>

std::vector<fs::path> Assets::_searchPaths;

void Assets::addSearchPath(const fs::path& path)
{
    if (std::find(_searchPaths.begin(), _searchPaths.end(), path) != _searchPaths.end()) return;
    _searchPaths.push_back(path);
}

fs::path Assets::Find(const std::string& name)
{
    for (auto& searchPath : _searchPaths) {
        auto filename = searchPath / name;
        if (fs::is_regular_file(filename))
            return filename;
    }
    auto fallback = fs::u8path("assets") / name; // fall-back to CWD
    return fallback;
}