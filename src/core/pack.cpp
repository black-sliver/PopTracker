#include "pack.h"
#include "fileutil.h"
#include "jsonutil.h"
#include <dirent.h>
#include "sha256.h"


using nlohmann::json;


std::vector<std::string> Pack::_searchPaths;


Pack::Pack(const std::string& path) : _zip(nullptr), _path(path)
{
    _loaded = std::chrono::system_clock::now();
    std::string s;
    if (fileExists(path)) {
        _zip = new Zip(path);
        auto root = _zip->list();
        if (root.size() == 1 && root[0].first == Zip::EntryType::DIR) {
            _zip->setDir(root[0].second);
        }
        std::string err;
        if (_zip->readFile("manifest.json", s, err)) {
            _manifest = parse_jsonc(s);
        } else {
            fprintf(stderr, "%s: could not read manifest.json: %s\n", path.c_str(), err.c_str());
        }
    } else {
        if (ReadFile("manifest.json", s)) {
            _manifest = parse_jsonc(s);
        }
    }
    if (_manifest.type() == json::value_t::object) {
        _uid = to_string(_manifest,"package_uid","");
        _name = to_string(_manifest,"name", _uid);
        _gameName = to_string(_manifest,"game_name", _name);
        _versionsURL = to_string(_manifest,"versions_url", "");
        _minPopTrackerVersion = to_string(_manifest, "min_poptracker_version", "");
    }
}

Pack::~Pack()
{
    if (_zip) delete _zip;
    _zip = nullptr;
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
                auto displayName = to_string(it.value(),"display_name",it.key());
                if (displayName.empty()) continue;
                variants.push_back({it.key(), displayName});
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
        _minPopTrackerVersion,
        variants
    };

    // Use the PopTracker platform override field
    std::string platform_override = to_string(_manifest, "platform_poptracker", "");
    if( platform_override != "" )
      info.platform = platform_override;

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

bool Pack::hasFile(const std::string& userfile) const
{
    std::string file;
    if (!sanitizePath(userfile, file)) return false;

    if (_zip) {
        if (!_variant.empty() && _zip->hasFile(_variant+"/"+file))
            return true;
        if (_zip->hasFile(file))
            return true;
    } else {
        if (!_variant.empty() && fileExists(os_pathcat(_path,_variant,file)))
            return true;
        if (fileExists(os_pathcat(_path,file)))
            return true;
    }
    return false;
}

bool Pack::ReadFile(const std::string& userfile, std::string& out) const
{
    std::string file;
    if (!sanitizePath(userfile, file)) return false;
    
    if (_zip) {
        if (!_variant.empty() && _zip->readFile(_variant+"/"+file, out))
            return true;
        if (_zip->readFile(file, out))
            return true;
        return false;
    } else {
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
}

void Pack::setVariant(const std::string& variant)
{
    // set variant and cache some common values
    _variant = variant;
    _variantName = variant; // fall-back
    if (_manifest.type() != json::value_t::object) return;
    auto& variants = _manifest["variants"];
    if (variants.is_object() && variants.find(variant) == variants.end() && variants.begin() != variants.end()) {
        _variant = variants.begin().key();
        _variantName = _variant;
    }
    _variantName = to_string(variants[_variant]["display_name"], _variantName);

    std::string s;
    if (ReadFile("settings.json", s)) {
        // TODO: allow extending from pack overrides
        _settings = parse_jsonc(s);
        if (_settings.type() != json::value_t::object)
            fprintf(stderr, "WARNING: invalid settings.json\n");
    }

    if (_settings.type() != json::value_t::object)
        _settings = json::object();
}

std::string Pack::getPlatform() const
{
    std::string platform = to_string(_manifest, "platform", "");

    // Use the PopTracker platform override field
    std::string platform_override = to_string(_manifest, "platform_poptracker", "");
    if( platform_override != "" )
        platform = platform_override;

    return platform;
}

std::string Pack::getVersion() const
{
    return to_string(_manifest,"package_version","");
}

bool Pack::variantHasFlag(const std::string& flag) const
{
    // jump through hoops to stay const
    auto variantsIt = _manifest.find("variants");
    if (variantsIt == _manifest.end()) return false;
    auto variantIt = variantsIt->find(_variant);
    if (variantIt == variantsIt->end()) return false;
    auto flagsIt = variantIt->find("flags");
    if (flagsIt == variantIt->end()) return false;
    // actually find flag
    const auto& flags = *flagsIt;
    for (const auto& f: flags) {
        if (f == flag) {
            printf("Pack: %s:%s has flag %s\n", _uid.c_str(), _variant.c_str(), flag.c_str());
            return true;
        }
    }
    printf("Pack: %s:%s has NO flag %s\n", _uid.c_str(), _variant.c_str(), flag.c_str());
    return false;
}

std::set<std::string> Pack::getVariantFlags() const
{
    std::set<std::string> set;
    // jump through hoops to stay const
    auto variantsIt = _manifest.find("variants");
    if (variantsIt == _manifest.end()) return set;
    auto variantIt = variantsIt->find(_variant);
    if (variantIt == variantsIt->end()) return set;
    auto flagsIt = variantIt->find("flags");
    if (flagsIt == variantIt->end()) return set;
    // actually iterate over flags
    const auto& flags = *flagsIt;
    for (const auto& f: flags) {
        if (f.is_string()) set.insert(f.get<std::string>());
    }
    return set;
}

static bool fileNewerThan(const struct stat* st, const std::chrono::system_clock::time_point& than)
{
    auto duration = std::chrono::seconds(st->st_mtime);
    return (std::chrono::system_clock::time_point(duration) > than);
}

static bool fileNewerThan(const std::string& path, const std::chrono::system_clock::time_point& than)
{
    std::chrono::system_clock::time_point tp;
    return (!getFileMTime(path, tp) || tp > than); // assume changed on error
}

static bool dirNewerThan(const char* path, const std::chrono::system_clock::time_point& than)
{
    DIR *d = opendir(path);
    if (!d) return true; // error -> assume changed
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL)
    {
        if (strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0) continue;
        auto f = os_pathcat(path, dir->d_name);
        struct stat st;
        if (stat(f.c_str(), &st) != 0) return true;
        if (S_ISDIR(st.st_mode) && dirNewerThan(f.c_str(), than)) return true;
        else if (!S_ISDIR(st.st_mode) && fileNewerThan(&st, than)) return true;
    }
    closedir(d);
    return false;
}

bool Pack::hasFilesChanged() const
{
    if (_zip) {
        if (fileNewerThan(_path, _loaded)) return true;
    } else {
        if (dirNewerThan(_path.c_str(), _loaded)) return true;
    }
    return false;
}

std::string Pack::getSHA256() const
{
    // NOTE: this is only possible for ZIPs. Returns empty string otherwise.
    if (_zip) {
        return SHA256_File(_path);
    }
    return "";
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
    std::sort(res.begin(), res.end(), [](const Pack::Info& lhs, const Pack::Info& rhs) {
        int n = strcasecmp(lhs.packName.c_str(), rhs.packName.c_str());
        if (n<0) return true;
        if (n>0) return false;
        int m = strcasecmp(lhs.version.c_str(), rhs.version.c_str());
        if (m<0) return true;
        return false;
    });
    return res;
}

Pack::Info Pack::Find(const std::string& uid, const std::string& version, const std::string& sha256)
{
    std::vector<Pack::Info> packs;
    for (auto& searchPath: _searchPaths) {
        DIR *d = opendir(searchPath.c_str());
        if (!d) continue;
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL)
        {
            Pack pack(os_pathcat(searchPath, dir->d_name));
            if (pack.isValid() && pack.getUID() == uid) {
                if (!sha256.empty()) {
                    // require exact match if hash is given
                    if ((version.empty() || pack.getVersion() == version) &&
                            strcasecmp(pack.getSHA256().c_str(), sha256.c_str()) == 0) {
                        return pack.getInfo(); // return exact match
                    }
                } else if (!version.empty() && pack.getVersion() == version) {
                    return pack.getInfo(); // return exact version match
                } else {
                    packs.push_back(pack.getInfo());
                }
            }
        }

        closedir(d);
    }
    if (!packs.empty()) {
        // fall back to latest version since an upgrade may have removed it
        std::sort(packs.begin(), packs.end(), [](const Pack::Info& lhs, const Pack::Info& rhs) {
            int m = strcasecmp(lhs.version.c_str(), rhs.version.c_str());
            return (m<0);
        });
        return packs.back();
    }
    return {};
}

void Pack::addSearchPath(const std::string& path)
{
    if (std::find(_searchPaths.begin(), _searchPaths.end(), path) != _searchPaths.end()) return;
    _searchPaths.push_back(path);
}

static std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

static std::string CleanUpPath(const std::string& path)
{
#ifdef WIN32
    char buf[_MAX_PATH];
    char* p = _fullpath(buf, path.c_str(), sizeof(buf));
    std::string s = p ? (std::string)p : path;
    s = ReplaceAll(ReplaceAll(s, "//", "/"), "\\\\", "\\");
    if (s[s.length()-1] == '/' || s[s.length()-1] == '\\')
        s.pop_back();
#else
    char* p = realpath(path.c_str(), nullptr);
    std::string s = p ? (std::string)p : path;
    if (p) free(p);
    p = nullptr;
    s = ReplaceAll(s, "//", "/");
    if (s[s.length()-1] == '/')
        s.pop_back();
#endif
    return s;
}

bool Pack::isInSearchPath(const std::string& uncleanPath)
{
    std::string path = CleanUpPath(uncleanPath);
    for (const auto& uncleanSearchPath: _searchPaths) {
        std::string searchPath = CleanUpPath(uncleanSearchPath) + OS_DIR_SEP;
        if (strncmp(path.c_str(), searchPath.c_str(), searchPath.length()) == 0)
            return true;
    }
    return false;
}
