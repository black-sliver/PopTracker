#include "pack.h"
#include "fileutil.h"
#include "jsonutil.h"
#include "version.h"
#include <dirent.h>
#include <stdlib.h>
#include "sha256.h"
#include "util.h"

#ifdef _WIN32
#include <wchar.h>
#endif


using nlohmann::json;


static bool sanitizePath(const std::string& userfile, std::string& file)
{
    if (userfile.empty())
        return false;
    // don't allow reference to parent directory
    auto p = userfile.find("../");
    if (p != userfile.npos) {
        if (p == 0 || (userfile[p-1] == '/' || userfile[p-1] == '\\'))
            return false;
    }
    p = userfile.find("..\\");
    if (p != userfile.npos) {
        if (p == 0 || (userfile[p-1] == '/' || userfile[p-1] == '\\'))
            return false;
    }
    // don't allow absolute paths on windows
    if (userfile.length()>1) {
        if (userfile[1]==':')
            return false;
        if (userfile[0]=='\\' && userfile[1]=='\\')
            return false;
    }
    // remove leading slashes
    file = userfile;
    while (file.length()>=2 && file[0]=='.' && (file[1]=='/' || file[1]=='\\'))
        file = file.substr(2);
    while (file[0]=='/' || file[0]=='\\')
        file = file.substr(1);
    return true;
}

template<class T>
static T replaceAll(T str, const T& from, const T& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

static fs::path cleanUpPath(const fs::path& path)
{
    using namespace std::string_literals;

#if defined WIN32 || defined _WIN32
    wchar_t buf[_MAX_PATH];
    auto p = _wfullpath(buf, path.c_str(), _MAX_PATH);
    std::wstring s = p ? p : path.c_str();
    s = replaceAll(replaceAll(s, L"//"s, L"/"s), L"\\\\"s, L"\\"s);
    if (s[s.length()-1] == L'/' || s[s.length()-1] == L'\\')
        s.pop_back();
#else
    char* p = realpath(path.c_str(), nullptr);
    std::string s = p ? p : path.c_str();
    if (p)
        free(p);
    p = nullptr;
    s = replaceAll(s, "//"s, "/"s);
    if (s[s.length()-1] == '/')
        s.pop_back();
#endif
    return s;
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

static bool fileNewerThan(const fs::path& path, const std::chrono::system_clock::time_point& than)
{
    std::chrono::system_clock::time_point tp;
    return (!getFileMTime(path, tp) || tp > than); // assume changed on error
}

static bool dirNewerThan(const fs::path& path, const std::chrono::system_clock::time_point& than)
{
    for (const auto& dirEntry: fs::recursive_directory_iterator{path}) {
        if ((dirEntry.is_regular_file() || dirEntry.is_symlink()) && fileNewerThan(dirEntry.path(), than))
            return true;
    }
    return false;
}


std::vector<fs::path> Pack::_searchPaths;
std::vector<fs::path> Pack::_overrideSearchPaths;


Pack::Pack(const fs::path& path) : _zip(nullptr), _path(path), _override(nullptr)
{
    _loaded = std::chrono::system_clock::now();
    std::string s;
    if (fs::is_regular_file(path)) {
        _zip = new Zip(path.u8string().c_str());
        auto root = _zip->list();
        if (root.size() == 1 && root[0].first == Zip::EntryType::DIR) {
            _zip->setDir(root[0].second);
        }
        std::string err;
        if (_zip->readFile("manifest.json", s, err)) {
            _manifest = parse_jsonc(s);
        } else {
            fprintf(stderr, "%s: could not read manifest.json: %s\n", sanitize_print(path).c_str(), err.c_str());
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
        _minPopTrackerVersion = Version{to_string(_manifest, "min_poptracker_version", "")};
        _targetPopTrackerVersion = Version{to_string(_manifest, "target_poptracker_version", "")};
    }

    if (!_uid.empty()) {
        for (const auto& path: _overrideSearchPaths) {
            auto overridePath = path / _uid;
            if (fs::is_directory(overridePath)) {
                _override = new Override(overridePath);
                break;
            }
        }
    }
}

Pack::~Pack()
{
    if (_override)
        delete _override;
    _override = nullptr;
    if (_zip)
        delete _zip;
    _zip = nullptr;
}

Pack::Info Pack::getInfo() const
{
    if (!isValid())
        return {};

    std::vector<Pack::VariantInfo> variants;
    auto vIt = _manifest.find("variants");
    if (vIt != _manifest.end()) {
        auto& v = vIt.value();
        if (v.type() == json::value_t::object) {
            for (auto it = v.begin(); it != v.end(); ++it) {
                auto displayName = to_string(it.value(),"display_name",it.key());
                if (displayName.empty())
                    continue;
                variants.push_back({it.key(), displayName});
            }
        }
    }
    if (variants.empty())
        variants.push_back({"","Standard"});

    Pack::Info info = {
        _path,
        _uid,
        to_string(_manifest, "package_version",""),
        getPlatform(),
        _gameName,
        _name,
        _minPopTrackerVersion,
        variants
    };

    return info;
}

bool Pack::hasFile(const std::string& userfile) const
{
    std::string file;
    if (!sanitizePath(userfile, file))
        return false;

    if (_zip) {
        if (!_variant.empty() && _zip->hasFile(_variant + "/" + file))
            return true;
        if (_zip->hasFile(file))
            return true;
    } else {
        if (!_variant.empty() && fs::is_regular_file(_path / fs::u8path(_variant) / fs::u8path(file)))
            return true;
        if (fs::is_regular_file(_path / fs::u8path(file)))
            return true;
    }
    return false;
}

bool Pack::ReadFile(const std::string& userfile, std::string& out, bool allowOverride) const
{
    std::string file;
    if (!sanitizePath(userfile, file))
        return false;

    if (_override && allowOverride) {
        if (file == "settings.json" && !_settings.is_null() && !_settings.empty()) {
            // reading settings.json from Lua will give the merged version from pack + user override
            out = _settings.dump();
            return true;
        }
        bool packHasVariantFile = false;
        if (!_variant.empty()) {
            if (_zip)
                packHasVariantFile = _zip->hasFile(_variant + "/" + file);
            else
                packHasVariantFile = fs::is_regular_file(_path / fs::u8path(_variant) / fs::u8path(file));
        }
        if (packHasVariantFile && _override->ReadFile(_variant + "/" + file, out))
            return true;
        else if ((!packHasVariantFile || _variant.empty()) && _override->ReadFile(file, out))
            return true;
    }

    if (_zip) {
        if (!_variant.empty() && _zip->readFile(_variant + "/" + file, out))
            return true;
        if (_zip->readFile(file, out))
            return true;
        return false;
    } else {
        out.clear();
        FILE* f = nullptr;
#ifdef _WIN32
        if (!_variant.empty()) {
            f = _wfopen((_path / fs::u8path(_variant) / fs::u8path(file)).c_str(), L"rb");
        }
        if (!f) {
            f = _wfopen((_path / fs::u8path(file)).c_str(), L"rb");
        }
#else
        if (!_variant.empty()) {
            f = fopen((_path / fs::u8path(_variant) / fs::u8path(file)).c_str(), "rb");
        }
        if (!f) {
            f = fopen((_path / fs::u8path(file)).c_str(), "rb");
        }
#endif
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
    if (_manifest.type() != json::value_t::object)
        return;
    auto& variants = _manifest["variants"];
    if (variants.is_object() && variants.find(variant) == variants.end() && variants.begin() != variants.end()) {
        _variant = variants.begin().key();
        _variantName = _variant;
    }
    _variantName = to_string(variants[_variant]["display_name"], _variantName);

    std::string s;
    if (ReadFile("settings.json", s, false)) {
        _settings = parse_jsonc(s);
        if (!_settings.is_object())
            fprintf(stderr, "WARNING: invalid settings.json\n");
    }

    if (_settings.type() != json::value_t::object)
        _settings = json::object();

    if (_override) {
        printf("overriding from %s\n", sanitize_print(_override->getPath()).c_str());
        if (_override->ReadFile("settings.json", s)) {
            json overrides = parse_jsonc(s);
            if (!overrides.is_object()) {
                fprintf(stderr, "WARNING: invalid settings.json override\n");
            } else {
                printf("settings.json overridden\n");
                _settings.update(overrides); // extend/override
            }
        }
    }

    const std::string filter = _settings.value("disabled_image_filter", "grey");
    commasplit(filter, _disabledImageFilter);
}

std::string Pack::getPlatform() const
{
    // For backwards compatibility; check the override field first
    std::string platform_override = to_string(_manifest, "platform_override", "");
    if( platform_override != "" )
        return platform_override;

    return to_string(_manifest, "platform", "");
}

std::string Pack::getVersion() const
{
    return to_string(_manifest,"package_version","");
}

bool Pack::variantHasFlag(const std::string& flag) const
{
    // jump through hoops to stay const
    auto variantsIt = _manifest.find("variants");
    if (variantsIt == _manifest.end())
        return false;
    auto variantIt = variantsIt->find(_variant);
    if (variantIt == variantsIt->end())
        return false;
    auto flagsIt = variantIt->find("flags");
    if (flagsIt == variantIt->end())
        return false;
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
        if (f.is_string())
            set.insert(f.get<std::string>());
    }
    return set;
}

bool Pack::hasFilesChanged() const
{
    if (_override && _override->hasFilesChanged(_loaded))
        return true;
    if (_zip)
        return fileNewerThan(_path, _loaded); // TODO: fs::last_write_time + time_point_cast once we use C+++20
    else
        return dirNewerThan(_path, _loaded);
}

std::string Pack::getSHA256() const
{
    // NOTE: this is only possible for ZIPs. Returns empty string otherwise.
    if (_zip)
        return SHA256_File(_path);
    return "";
}

std::vector<Pack::Info> Pack::ListAvailable()
{
    std::vector<Pack::Info> res;
    for (auto& searchPath: _searchPaths) {
        if (!fs::is_directory(searchPath))
            continue;
        for (auto const& dirEntry : fs::directory_iterator{searchPath}) {
            Pack pack(dirEntry.path());
            if (!pack.isValid())
                continue;
            res.push_back(pack.getInfo());
        }
    }

    std::sort(res.begin(), res.end(), [](const Pack::Info& lhs, const Pack::Info& rhs) {
        int n = strcasecmp(lhs.packName.c_str(), rhs.packName.c_str());
        if (n<0)
            return true;
        if (n>0)
            return false;
        return Version(lhs.version) < Version(rhs.version);
    });

    return res;
}

Pack::Info Pack::Find(const std::string& uid, const std::string& version, const std::string& sha256)
{
    std::vector<Pack::Info> packs;
    for (auto& searchPath: _searchPaths) {
        if (!fs::is_directory(searchPath))
            continue;
        for (auto const& dirEntry : fs::directory_iterator{searchPath}) {
            Pack pack(dirEntry.path());
            if (!pack.isValid() || pack.getUID() != uid)
                continue;
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

    if (!packs.empty()) {
        // fall back to latest version since an upgrade may have removed it
        std::sort(packs.begin(), packs.end(), [](const Pack::Info& lhs, const Pack::Info& rhs) {
            return Version(lhs.version) < Version(rhs.version);
        });
        return packs.back();
    }

    return {};
}

static void addPath(const fs::path& path, std::vector<fs::path>& paths)
{
#if !defined WIN32 && !defined _WIN32
    // TODO: canonical?
    char* tmp = realpath(path.c_str(), NULL);
    if (tmp) {
        std::string real = tmp;
        free(tmp);
        if (std::find(paths.begin(), paths.end(), real) != paths.end())
            return;
        paths.push_back(real);
    } else
#else
    // TODO: canonical?
    auto tmp = _wfullpath(nullptr, path.c_str(), 1024);
    if (tmp) {
        auto cmp = [tmp](const fs::path& p) { return wcsicmp(tmp, p.c_str()) == 0; };
        if (std::find_if(paths.begin(), paths.end(), cmp) != paths.end())
            return;
        paths.push_back(tmp);
        free(tmp);
    } else
#endif
    {
        if (std::find(paths.begin(), paths.end(), path) != paths.end())
            return;
        paths.push_back(path);
    }
}

void Pack::addSearchPath(const fs::path& path)
{
    addPath(path, _searchPaths);
}

bool Pack::isInSearchPath(const fs::path& uncleanPath)
{
    auto path = cleanUpPath(uncleanPath);
    for (const auto& uncleanSearchPath: _searchPaths) {
        auto searchPath = cleanUpPath(uncleanSearchPath);
        // TODO: .make_preferred().canonical() instead
        if (fs::is_sub_path(path, searchPath))
            return true;
    }
    return false;
}

const std::vector<fs::path>& Pack::getSearchPaths()
{
    return _searchPaths;
}

void Pack::addOverrideSearchPath(const fs::path& path)
{
    addPath(path, _overrideSearchPaths);
}

Pack::Override::Override(const fs::path& path)
    : _path(path)
{
}

Pack::Override::~Override()
{
}

bool Pack::Override::ReadFile(const std::string& userfile, std::string& out) const
{
    std::string file;
    if (!sanitizePath(userfile, file))
        return false;

    out.clear();
    FILE* f =
#ifdef _WIN32
        _wfopen((_path / fs::u8path(file)).c_str(), L"rb");
#else
        fopen((_path / fs::u8path(file)).c_str(), "rb");
#endif
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

bool Pack::Override::hasFilesChanged(std::chrono::system_clock::time_point since) const
{
    return dirNewerThan(_path, since);
}
