#include "pack.h"
#include "fileutil.h"
#include "jsonutil.h"
#include <dirent.h>
using nlohmann::json;


std::vector<std::string> Pack::_searchPaths;


Pack::Pack(const std::string& path) : _path(path)
{
    std::string s;
    if (ReadFile("manifest.json", s)) {
        _manifest = parse_jsonc(s);
        if (_manifest.type() == json::value_t::object) {
            _uid = to_string(_manifest,"package_uid","");
            _name = to_string(_manifest,"name", _uid);
            _gameName = to_string(_manifest,"game_name", _name);
        }
    }
}

Pack::Info Pack::getInfo() const
{ 
    if (!isValid()) return {};
    
    std::vector<Pack::VariantInfo> variants;
    auto vIt = _manifest.find("variants");
    if (vIt != _manifest.end()) {
        auto& v = vIt.value();
        if (v.type() == json::value_t::object) {
            for (auto it = v.begin(); it != v.end(); ++it) {
                variants.push_back({it.key(),to_string(it.value(),"display_name",it.key())});
            }
        }
    }
    if (variants.empty()) variants.push_back({"","Standard"});
    
    Pack::Info info = {
        _path,
        _uid,
        to_string(_manifest, "package_version",""),
        to_string(_manifest, "platform",""),
        _gameName,
        _name,
        variants
    };
    return info;
}

bool sanitizePath(const std::string& userfile, std::string& file)
{
    if (userfile.empty()) return false;
    // don't allow reference to parent directory
    auto p = userfile.find("../");
    if (p != userfile.npos) {
        if (p == 0 || (userfile[p-1] == '/' || userfile[p-1] == '\\')) return false;
    }
    p = userfile.find("..\\");
    if (p != userfile.npos) {
        if (p == 0 || (userfile[p-1] == '/' || userfile[p-1] == '\\')) return false;
    }
    // don't allow absolute paths on windows
    if (userfile.length()>1) {
        if (userfile[1]==':') return false;
        if (userfile[0]=='\\' && userfile[1]=='\\') return false;
    }
    // remove leading slashes
    file = userfile;   
    while (file.length()>=2 && file[0]=='.' && (file[1]=='/' || file[1]=='\\')) file = file.substr(2);
    while (file[0]=='/' || file[0]=='\\') file = file.substr(1); 
    return true;
}

bool Pack::ReadFile(const std::string& userfile, std::string& out) const
{
    std::string file;
    if (!sanitizePath(userfile, file)) return false;
    
    out.clear();
    FILE* f = nullptr;
    if (!_variant.empty()) {
        f = fopen(os_pathcat(_path,_variant,file).c_str(), "rb");
    }
    if (!f) {
        f = fopen(os_pathcat(_path,file).c_str(), "rb");
    }
    if (!f) {
        return false;
    }
    while (!feof(f)) {
        char buf[4096];
        size_t sz = fread(buf, 1, sizeof(buf), f);
        if (ferror(f)) goto read_err;
        out += std::string(buf, sz);
    }
    fclose(f);
    return true;
read_err:
    fclose(f);
    out.clear();
    return false;
}

void Pack::setVariant(const std::string& variant)
{
    // set variant and cache some common values
    _variant = variant;
    _variantName = variant; // fall-back
    if (_manifest.type() != json::value_t::object) return;
    _variantName = to_string(_manifest["variants"][_variant]["display_name"], _variantName);
}

std::string Pack::getPlatform() const
{
    return to_string(_manifest,"platform","");
}
std::string Pack::getVersion() const
{
    return to_string(_manifest,"package_version","");
}
bool Pack::variantHasFlag(const std::string& flag)
{
    if (_manifest.type() != json::value_t::object) return false;
    auto& flags = _manifest["variants"][_variant]["flags"];
    if (flags.type() != json::value_t::array) {
        printf("Pack: %s:%s has no flags (%s)\n", _uid.c_str(), _variant.c_str(), flag.c_str());
        return false;
    }
    for (const auto& f: flags) {
        if (f == flag) {
            printf("Pack: %s:%s has flag %s\n", _uid.c_str(), _variant.c_str(), flag.c_str());
            return true;
        }
    }
    printf("Pack: %s:%s has NO flag %s\n", _uid.c_str(), _variant.c_str(), flag.c_str());
    return false;
}

std::vector<Pack::Info> Pack::ListAvailable()
{
    std::vector<Pack::Info> res;
    for (auto& searchPath: _searchPaths) {
        DIR *d = opendir(searchPath.c_str());
        if (!d) continue;
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL)
        {
            Pack pack(os_pathcat(searchPath, dir->d_name));
            if (!pack.isValid()) continue;
            res.push_back(pack.getInfo());
        }

        closedir(d);
    }
    return res;
}
Pack::Info Pack::Find(std::string uid, std::string version)
{
    for (auto& searchPath: _searchPaths) {
        DIR *d = opendir(searchPath.c_str());
        if (!d) continue;
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL)
        {
            Pack pack(os_pathcat(searchPath, dir->d_name));
            if (pack.isValid() && pack.getUID() == uid && (version.empty() || pack.getVersion()==version))
                return pack.getInfo();
        }

        closedir(d);
    }
    return {};
}
void Pack::addSearchPath(const std::string& path)
{
    if (std::find(_searchPaths.begin(), _searchPaths.end(), path) != _searchPaths.end()) return;
    _searchPaths.push_back(path);
}