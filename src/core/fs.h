#pragma once

//#define NO_STD_FILESYSTEM // define to force using boost::filesystem

#if !defined(NO_STD_FILESYSTEM) && !defined(FS_USE_STD_FILESYSTEM)
#ifdef __has_include
#   if __has_include(<filesystem>)
#       define FS_USE_STD_FILESYSTEM
#   endif
#endif
#endif


#include <cstring>
#include <climits>

#ifndef _WIN32
#include <pwd.h>
#include <unistd.h>
#endif

#if defined __APPLE__
#include <mach-o/dyld.h>
#elif defined _WIN32
#include <windows.h>
#include <shlobj.h>
#endif


#ifdef FS_USE_STD_FILESYSTEM

#include <filesystem>
#include <functional>

namespace fs {
    using std::filesystem::path;
    using std::filesystem::current_path;
    using std::filesystem::is_regular_file;
    using std::filesystem::equivalent;
    using std::filesystem::u8path;
    using std::filesystem::create_directories;
    using std::filesystem::exists;
    using std::filesystem::remove;
    using std::filesystem::remove_all;
    using std::filesystem::rename;
    using std::filesystem::perms;
    using std::filesystem::status;
    using std::filesystem::is_directory;
    using std::filesystem::last_write_time;
    using std::filesystem::directory_iterator;
    using std::filesystem::recursive_directory_iterator;
    using std::filesystem::copy;
    using std::filesystem::copy_options;
    using std::filesystem::file_size;
    using std::filesystem::temp_directory_path;
    using std::error_code;
}

#else

#include <boost/filesystem.hpp>
#include <codecvt>

namespace fs {
    using boost::filesystem::current_path;
    using boost::filesystem::is_regular_file;
    using boost::filesystem::equivalent;
    using boost::filesystem::create_directories;
    using boost::filesystem::exists;
    using boost::filesystem::remove;
    using boost::filesystem::remove_all;
    using boost::filesystem::rename;
    using boost::filesystem::perms;
    using boost::filesystem::status;
    using boost::filesystem::is_directory;
    using boost::filesystem::last_write_time;
    using boost::filesystem::directory_iterator;
    using boost::filesystem::recursive_directory_iterator;
    using boost::filesystem::copy;
    using boost::filesystem::copy_options;
    using boost::filesystem::file_size;
    using boost::system::error_code;

    class path : public boost::filesystem::path {
    public:
        path() {}
        path(const std::string& s) : boost::filesystem::path(s) {}
        path(const char* s) : boost::filesystem::path(s) {}
        path(const boost::filesystem::path& path) : boost::filesystem::path(path) {}

        std::string u8string() const
        {
#ifdef _WIN32
#error "Not implemented"
#else
            // we assume utf8 on non-windows
            return string();
#endif
        }

        std::string string() const
        {
#ifdef _WIN32
#error "Not implemented"
#else
            // we assume utf8 on non-windows
            return boost::filesystem::path::string();
#endif
        }

        path parent_path() const
        {
            return boost::filesystem::path::parent_path();
        }

        path filename() const
        {
            return boost::filesystem::path::filename();
        }
    };

    inline path operator/(path lhs, boost::filesystem::path const& rhs)
    {
        lhs.append(rhs);
        return lhs;
    }

    inline path operator/(path lhs, const std::string& rhs)
    {
        lhs.append(rhs);
        return lhs;
    }

    inline path operator/(path lhs, const char* rhs)
    {
        lhs.append(rhs);
        return lhs;
    }

    template<typename SOURCE>
    inline path u8path(const SOURCE& s)
    {
#ifdef _WIN32
        auto wideStr = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(s);
        return path(wideStr);
#else
        // we assume utf8 on non-windows
        return s;
#endif
    }

    inline path temp_directory_path()
    {
        return boost::filesystem::temp_directory_path();
    }
}

#endif


// extensions to std::filesystem
namespace  fs {
    inline path app_path()
    {
#if defined _WIN32
        wchar_t result[_MAX_PATH+1];
        if (GetModuleFileNameW(nullptr, result, _MAX_PATH) < 1) {
            fprintf(stderr, "Warning: could not get app path!\n");
            return "";
        }
        result[_MAX_PATH] = 0;
        wchar_t* slash = wcsrchr(result, L'\\');
#elif defined __APPLE__
        uint32_t bufsize = PATH_MAX;
        char result[PATH_MAX+1];
        memset(result, 0, sizeof(result)); // to make valgrind happy
        if(_NSGetExecutablePath(result, &bufsize) == -1) {
            fprintf(stderr, "Warning: could not get app path!\n");
            return "";
        }
        result[PATH_MAX] = 0;

        char* slash = strrchr(result, '/');
#else
        char result[PATH_MAX+1];
        memset(result, 0, sizeof(result)); // to make valgrind happy
        if (readlink("/proc/self/exe", result, PATH_MAX)<0) {
            fprintf(stderr, "Warning: could not get app path!\n");
            return "";
        }
        result[PATH_MAX] = 0;
        char* slash = strrchr(result, '/');
#endif
        if (!slash)
            return "";
        *slash = 0;
        return result;
    }

    inline path home_path()
    {
#ifdef _WIN32
        wchar_t result[_MAX_PATH+1];
        if (SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, SHGFP_TYPE_CURRENT, result) != 0) {
            fprintf(stderr, "Warning: could not get Home path!\n");
            return "";
        }
        result[_MAX_PATH] = 0;
        return result;
#else
        const char* res = getenv("HOME");
        if (res && *res) {
            return res;
        } else {
            struct passwd pwd;
            struct passwd *ppwd;
            char buf[4096];
            if (getpwuid_r(getuid(), &pwd, buf, sizeof(buf), &ppwd) != 0 || !ppwd)
                return "";
            return ppwd->pw_dir;
        }
#endif
    }

    inline path documents_path()
    {
#ifdef _WIN32
        wchar_t result[_MAX_PATH+1];
        if (SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, result) != 0) {
            fprintf(stderr, "Warning: could not get My Documents path!\n");
            return "";
        }
        result[_MAX_PATH] = 0;
        return result;
#else
        // this returns XDG_DOCUMENTS_DIR if defined, HOME otherwise
        const char* res = getenv("XDG_DOCUMENTS_DIR");
        auto home = home_path();
        if (res && *res && res != home)
            return res;
        fs::error_code ec;
        auto documents = home / "Documents";
        if (fs::is_directory(documents, ec))
            return documents;
        return home;
#endif
    }

    inline bool is_sub_path(path p, const path& root)
    {
        static const path emptyPath;
        while (p != emptyPath) {
            fs::error_code ec;
            if (equivalent(p, root, ec)) {
                return true;
            }
            path parent = p.parent_path();
            if (parent == p) {
                break;
            }
            p = parent;
        }
        return false;
    }
}
