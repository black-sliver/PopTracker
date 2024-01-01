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

static std::string os_basename(const std::string& filename)
{
    auto p = filename.rfind("/");
    if (OS_DIR_SEP != '/') {
        auto p2 = filename.rfind(OS_DIR_SEP);
        if (p == filename.npos || (p2 != filename.npos && p2>p))
            p = p2;
    }
    return filename.substr(p+1);
}

#include <sys/stat.h>
static inline bool fileExists(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISREG(st.st_mode);
}

static inline bool fileExists(const std::string& path)
{
    return fileExists(path.c_str());
}

static inline bool dirExists(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

static inline bool dirExists(const std::string& path)
{
    return dirExists(path.c_str());
}

static inline bool pathExists(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0) return false;
    return true;
}

static inline bool pathExists(const std::string& path)
{
    return pathExists(path.c_str());
}

static inline bool isWritable(const char* path)
{
    return (access(path, W_OK) == 0);
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

static std::string getConfigPath(const std::string& appname="", const std::string& filename="", bool isPortable=false)
{
    std::string res;
    if (isPortable) {
        res = os_pathcat(getAppPath(), "portable-config");
    } else {
        char result[_MAX_PATH+1];
        if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, result) != 0) {
            fprintf(stderr, "Warning: could not get AppData path!\n");
            res = getHomePath();
        } else {
            result[_MAX_PATH] = 0;
            res = result;
        }
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

static std::string getConfigPath(const std::string& appname="", const std::string& filename="", bool isPortable=false)
{
    // this returns XDG_CONFIG_DIR[/appname] if defined, HOME/.config[/appname] otherwise
    std::string res;
    if (isPortable) {
        res = os_pathcat(getAppPath(), ".portable-config");
    } else {
        const char* tmp = getenv("XDG_CONFIG_DIR");
        if (tmp && *tmp) {
            res = tmp;
        } else {
            res = os_pathcat(getHomePath(), ".config");
        }
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

static inline int mkdir_recursive(const std::string& dir, mode_t mode=0750) {
    return mkdir_recursive(dir.c_str(), mode);
}

static int os_copyfile(const char* src, const char* dst)
{
#ifdef WIN32
    if (!CopyFileA(src, dst, 0)) return -1;
    return 0;
#else
    int input, output;
    if ((input = open(src, O_RDONLY)) == -1)
    {
        return -1;
    }
    if ((output = creat(dst, 0660)) == -1)
    {
        close(input);
        return -1;
    }

#if defined(__APPLE__) || defined(__FreeBSD__)
    int result = fcopyfile(input, output, 0, COPYFILE_ALL);
#else
    off_t bytesCopied = 0;
    struct stat fileinfo = {0};
    fstat(input, &fileinfo);
    int result = sendfile(output, input, &bytesCopied, fileinfo.st_size);
#endif

    close(input);
    close(output);

    return result;
#endif
}

static bool copy_recursive(const char* src, const char* dst);
static bool copy_recursive(const char* src, const char* dst)
{
    if (fileExists(src)) {
        return (os_copyfile(src, dst) >= 0);
    } else if (dirExists(src)) {
        mkdir_recursive(dst);
        DIR *d = opendir(src);
        if (!d) return false;
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL)
        {
            if (strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0) continue;
            auto fsrc = os_pathcat(src, dir->d_name);
            auto fdst = os_pathcat(dst, dir->d_name);
            if (!copy_recursive(fsrc.c_str(), fdst.c_str())) return false;
        }
        closedir(d);
        return true;
    }
    return false;
}

static bool getFileMTime(const char* path, std::chrono::system_clock::time_point& tp)
{
    struct stat st;
    if (stat(path, &st)) return false;
    auto duration = std::chrono::seconds(st.st_mtime);
    tp = std::chrono::system_clock::time_point(duration);
    return true;
}

static bool getFileMTime(const std::string& path, std::chrono::system_clock::time_point& tp)
{
    return getFileMTime(path.c_str(), tp);
}

#ifdef WIN32
// for now we use ANSI paths on windows, so we have to convert ANSI -> UTF8
static std::string pathToUTF8(const std::string& s)
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

static std::string pathFromUTF8(const std::string& s)
{
    std::wstring wbuf;
    std::string res;
    // utf8 -> wstring
    int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    if (wlen < 1) // error
        return res;
    wbuf.resize(wlen); // resize to incl NUL, since the implicit NUL can not be written
    wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, wbuf.data(), wbuf.size());
    if (wlen < 1) // error
        return res;
    wbuf.resize(wlen - 1); // cut terminating NUL
    // wstring -> ansi
    int len = WideCharToMultiByte(CP_ACP, 0, wbuf.c_str(), -1, NULL, 0, NULL, NULL);
    if (len < 1) // error
        return res;
    res.resize(len);
    len = WideCharToMultiByte(CP_ACP, 0, wbuf.c_str(), -1, res.data(), res.size(), NULL, NULL);
    if (len < 1) // error
        res.resize(0);
    else
        res.resize(len - 1); // cut terminating NUL
    return res;
}
#else // non-WIN32
// on non-windows paths are expected to be UTF8
#define pathToUTF8(s) (s)
#define pathFromUTF8(s) (s)
#endif

#endif // _CORE_FILEUTIL_H
