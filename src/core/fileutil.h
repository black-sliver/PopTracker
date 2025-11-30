/*
 fileutil.h
 (c) 2021 black-sliver, sbzappa
 */

#ifndef _CORE_FILEUTIL_H
#define _CORE_FILEUTIL_H


#include <stdio.h>
#include <string>
#include <chrono>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include "fs.h"
#include "util.h"
#include <cwchar>

#if defined(__APPLE__) && !defined(MACOS)
#define MACOS
#endif

#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif

#if defined(MACOS) || defined(__FreeBSD__)
#include <copyfile.h>
#elif !defined(WIN32)
#include <sys/sendfile.h>
#endif

static bool readFile(const fs::path& file, std::string& out)
{
    out.clear();
#ifdef _WIN32
    FILE* f = _wfopen(file.c_str(), L"rb");
#else
    FILE* f = fopen(file.c_str(), "rb");
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

static bool writeFile(const fs::path& file, const std::string& data)
{
#ifdef _WIN32
    FILE* f = _wfopen(file.c_str(), L"wb");
#else
    FILE* f = fopen(file.c_str(), "wb");
#endif
    if (!f) {
        fprintf(stderr, "Could not open file \"%s\" for writing: %s\n", sanitize_print(file).c_str(), strerror(errno));
        return false;
    }
    size_t res = fwrite(data.c_str(), 1, data.length(), f);
    fclose(f);
    return res;
}


#ifdef WIN32
static const char OS_DIR_SEP = '\\';
#else
static const char OS_DIR_SEP = '/';
#endif

static inline bool isWritable(const char* path)
{
    return (access(path, W_OK) == 0);
}

#ifdef _WIN32
static inline bool isWritable(const wchar_t* path)
{
    return (_waccess(path, W_OK) == 0);
}
#endif

static inline bool isWritable(const fs::path& path)
{
    return isWritable(path.c_str());
}

static inline bool isWritable(const std::string& path)
{
    return isWritable(path.c_str());
}

static std::string getCwd()
{
   char cwd[PATH_MAX];
   if (getcwd(cwd, sizeof(cwd)) != NULL) {
       return cwd;
   } else {
       return "";
   }
}

#ifdef WIN32
#include <libloaderapi.h> // GetModileFileNameA
#include <libgen.h> // dirname
#include <shlobj.h>

static fs::path getConfigPath(const std::string& appname="", const std::string& filename="", bool isPortable=false)
{
    fs::path res;
    if (isPortable) {
        res = fs::app_path() / "portable-config";
    } else {
        wchar_t result[_MAX_PATH+1];
        if (SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, result) != 0) {
            fprintf(stderr, "Warning: could not get AppData path!\n");
            res = fs::home_path();
        } else {
            result[_MAX_PATH] = 0;
            res = result;
        }
    }

    if (!appname.empty() && !filename.empty())
        return res / appname / filename;
    if (!appname.empty())
        return res / appname;
    if (!filename.empty())
        return res / filename;
    return res;
}

#else

static fs::path getConfigPath(const std::string& appname="", const std::string& filename="", bool isPortable=false)
{
    // this returns XDG_CONFIG_DIR[/appname] if defined, HOME/.config[/appname] otherwise
    fs::path res;
    if (isPortable) {
        res = fs::app_path() / ".portable-config";
    } else {
        const char* tmp = getenv("XDG_CONFIG_DIR");
        if (tmp && *tmp) {
            res = tmp;
        } else {
            res = fs::home_path() / ".config";
        }
    }

    if (!appname.empty() && !filename.empty())
        return res / appname / filename;
    else if (!appname.empty())
        return res / appname;
    else if (!filename.empty())
        return res / filename;
    return res;
}
#endif

static bool copy_recursive(const fs::path& src, const fs::path& dst, fs::error_code& ec)
{
    fs::copy(src, dst, fs::copy_options::overwrite_existing | fs::copy_options::recursive, ec);
    return !ec;
}

#ifdef _WIN32
static bool getFileMTime(const wchar_t* path, std::chrono::system_clock::time_point& tp)
{
    struct _stat64 st;
    if (_wstat64(path, &st))
        return false;
    auto duration = std::chrono::seconds(st.st_mtime);
    tp = std::chrono::system_clock::time_point(duration);
    return true;
}
#else
static bool getFileMTime(const char* path, std::chrono::system_clock::time_point& tp)
{
    struct stat st;
    if (stat(path, &st)) return false;
    auto duration = std::chrono::seconds(st.st_mtime);
    tp = std::chrono::system_clock::time_point(duration);
    return true;
}
#endif

static bool getFileMTime(const fs::path& path, std::chrono::system_clock::time_point& tp)
{
    return getFileMTime(path.c_str(), tp);
}

#define pathFromUTF8(s) fs::u8path<std::string>(s)

#ifdef WIN32
// we use ANSI paths on windows in some places, so we have to convert ANSI -> UTF8
static std::string localToUTF8(const std::string& s)
{
    std::wstring wbuf;
    std::string res;
    // ansi -> wstring
    int wlen = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, NULL, 0);
    if (wlen < 1) // error
        return res;
    wbuf.resize(wlen); // resize to incl NUL, since the implicit NUL can not be written
    wlen = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, wbuf.data(), wbuf.size());
    if (wlen < 1) // error
        return res;
    wbuf.resize(wlen - 1); // cut terminating NUL
    // wstring -> utf8
    int len = WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), -1, NULL, 0, NULL, NULL);
    if (len < 1) // error
        return res;
    res.resize(len);
    len = WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), -1, res.data(), res.size(), NULL, NULL);
    if (len < 1) // error
        res.resize(0);
    else
        res.resize(len - 1); // cut terminating NUL
    return res;
}
#else // non-WIN32
// on non-windows paths are expected to be UTF8
#define localToUTF8(s) (s)
#endif

#endif // _CORE_FILEUTIL_H
