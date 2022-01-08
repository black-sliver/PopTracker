/*
 fileutil.h
 (c) 2021 black-sliver, sbzappa
 */

#ifndef _CORE_FILEUTIL_H
#define _CORE_FILEUTIL_H


#include <stdio.h>
#include <string>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#if defined(__APPLE__) && !defined(MACOS)
#define MACOS
#endif

#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif


static bool readFile(const std::string& file, std::string& out)
{
    out.clear();
    FILE* f = fopen(file.c_str(), "rb");
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

static bool writeFile(const std::string& file, const std::string& data)
{
    FILE* f = fopen(file.c_str(), "wb");
    if (!f) {
        fprintf(stderr, "Cloud not open file \"%s\" for writing: %s\n", file.c_str(), strerror(errno));
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
    
static std::string os_pathcat(const std::string& a, const std::string& b)
{
    if (a.empty()) return b;
    if (a[a.length()-1] == OS_DIR_SEP) return a+b;
    return a + OS_DIR_SEP + b;
}
template <typename... Args>
static std::string os_pathcat(const std::string a, const std::string b, Args... args) {
    return os_pathcat(os_pathcat(a,b), args...);
}

static std::string os_dirname(const std::string& filename)
{
    auto p = filename.rfind("/");
    if (OS_DIR_SEP != '/') {
        auto p2 = filename.rfind(OS_DIR_SEP);
        if (p == filename.npos || (p2 != filename.npos && p2>p))
            p = p2;
    }
    return filename.substr(0, p);
}

#include <sys/stat.h>
static inline bool fileExists(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return S_ISREG(st.st_mode);
}
static inline bool pathExists(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) return false;
    return true;
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
static std::string getAppPath()
{
    char result[_MAX_PATH+1];
    if (GetModuleFileNameA(NULL,result,_MAX_PATH) < 1) {
        fprintf(stderr, "Warning: could not get app path!\n");
        return "";
    }
    result[_MAX_PATH] = 0;
    char* slash = strrchr(result, '\\');
    if (!slash) return "";
    *slash = 0;
    return result;
}
#include <shlobj.h>
static std::string getHomePath()
{
    char result[_MAX_PATH+1];
    if (SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, result) != 0) {
        fprintf(stderr, "Warning: could not get Home path!\n");
        return "";
    }
    result[_MAX_PATH] = 0;
    return result;
}
static std::string getDocumentsPath()
{
    char result[_MAX_PATH+1];
    if (SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, result) != 0) {
        fprintf(stderr, "Warning: could not get My Documents path!\n");
        return "";
    }
    result[_MAX_PATH] = 0;
    return result;
}
static std::string getConfigPath(const std::string& appname="", const std::string& filename="")
{
    std::string res;
    char result[_MAX_PATH+1];
    if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, result) != 0) {
        fprintf(stderr, "Warning: could not get AppData path!\n");
        res = getHomePath();
    } else {
        result[_MAX_PATH] = 0;
        res = result;
    }
    
    if (!appname.empty() && !filename.empty())
        return os_pathcat(res, appname, filename);
    else if (!appname.empty())
        return os_pathcat(res, appname);
    else if (!filename.empty())
        return os_pathcat(res, filename);
    return res;
}
#else
#include <libgen.h> // dirname
#include <sys/types.h> // struct passwd
#include <pwd.h> // getpwuid

#ifdef MACOS
#include <mach-o/dyld.h>
static std::string getAppPath()
{
    uint32_t bufsize = PATH_MAX;
    char result[PATH_MAX+1]; memset(result, 0, sizeof(result)); // to make valgrind happy
    if(_NSGetExecutablePath(result, &bufsize) == -1) {
        fprintf(stderr, "Warning: could not get app path!\n");
        return "";
    }
    result[PATH_MAX]=0;

    char* slash = strrchr(result, '/');
    if (!slash) return "";
    *slash = 0;
    return result;
}
#else
static std::string getAppPath()
{
    char result[PATH_MAX+1]; memset(result, 0, sizeof(result)); // to make valgrind happy
    if (readlink("/proc/self/exe", result, PATH_MAX)<0) {
        fprintf(stderr, "Warning: could not get app path!\n");
        return "";
    }
    result[PATH_MAX]=0;
    char* slash = strrchr(result, '/');
    if (!slash) return "";
    *slash = 0;
    return result;
}
#endif

static std::string getHomePath()
{
    const char* res = getenv("HOME");
    if (res && *res) return res;
    else {
        struct passwd pwd;
        struct passwd *ppwd;
        char buf[4096];
        if (getpwuid_r(getuid(), &pwd, buf, sizeof(buf), &ppwd) != 0 || !ppwd) return "";
        return ppwd->pw_dir;
    }
}
static std::string getDocumentsPath()
{
    // this returns XDG_DOCUMENTS_DIR if defined, HOME otherwise
    const char* res = getenv("XDG_DOCUMENTS_DIR");
    if (res && *res) return res;
    return getHomePath();
}
static std::string getConfigPath(const std::string& appname="", const std::string& filename="")
{
    // this returns XDG_CONFIG_DIR[/appname] if defined, HOME/.config[/appname] otherwise
    std::string res;
    const char* tmp = getenv("XDG_CONFIG_DIR");
    if (tmp && *tmp) {
        res = tmp;
    } else {
        res = os_pathcat(getHomePath(), ".config");
    }
    
    if (!appname.empty() && !filename.empty())
        return os_pathcat(res, appname, filename);
    else if (!appname.empty())
        return os_pathcat(res, appname);
    else if (!filename.empty())
        return os_pathcat(res, filename);
    return res;
}
#endif

#include <sys/stat.h>
#include <sys/types.h>
#ifdef WIN32
#define MKDIR(a,b) mkdir(a)
#else
#define MKDIR mkdir
#endif
static int mkdir_recursive(const char *dir, mode_t mode=0750) {
    if (!dir) {
        errno = EINVAL;
        return -1;
    }
    size_t len = strlen(dir);
    char *tmp = (char*)malloc(len+1);
    if (!tmp) {
        errno = ENOMEM;
        return -1;
    }
    memcpy(tmp, dir, len+1);
    if(tmp[len-1]=='/' || tmp[len-1]==OS_DIR_SEP) tmp[len-1] = 0;
    
    for(char *p = tmp + 1; *p; p++) {
        if(*p=='/' || *p==OS_DIR_SEP) {
            char c = *p;
            *p = 0;
            MKDIR(tmp, mode);
            *p = c;
        }
    }
    int res = MKDIR(tmp, mode);
    free(tmp);
    return res;
}

#endif // _CORE_FILEUTIL_H
