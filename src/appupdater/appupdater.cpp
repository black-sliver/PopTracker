#include "appupdater.hpp"
#include <cstdio> // TODO: move to fmt::print
#include <fmt/xchar.h>
#include "../poptracker.h"
#include "../core/jsonutil.h"
#include "../gh/api/releases.hpp"
#include "../http/http.h"
#include "../uilib/dlg.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif


using namespace Ui;

namespace pop {

#ifdef _WIN32
static std::string GetWindowsErrorString(const DWORD errorCode)
{
    LPWSTR messageBuffer = nullptr;
    try {
    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer,
        0,
        nullptr);

        if (size == 0) {
            return "Unknown";
        }

        const auto utf8Size = WideCharToMultiByte(CP_UTF8, 0, messageBuffer, size, nullptr, 0, nullptr, nullptr);
        std::string utf8Message(utf8Size, '\0');
        if (WideCharToMultiByte(CP_UTF8, 0, messageBuffer, size, utf8Message.data(), utf8Size, nullptr, nullptr) < 1) {
            return "Unknown";
        }

        LocalFree(messageBuffer);

        return utf8Message;
    } catch (...) {
        LocalFree(messageBuffer);
        throw;
    }
}
#endif

static void openWebsite(const std::string& url)
{
#if defined _WIN32 || defined WIN32
    ShellExecuteA(nullptr, "open", url.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#else
    const auto pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Update: fork failed\n");
    } else if (pid == 0) {
#if defined __APPLE__ || defined MACOS
        const char* exe = "open";
#else
        const auto exe = "xdg-open";
#endif
        execlp(exe, exe, url.c_str(), nullptr);
        std::string msg = "Could not launch browser!\n";
        msg += exe;
        msg += ": ";
        msg += strerror(errno);
        fprintf(stderr, "Update: %s\n", msg.c_str());
        Dlg::MsgBox("PopTracker", msg,
                Dlg::Buttons::OK, Dlg::Icon::Error);
        exit(0);
    }
#endif
}

void AppUpdater::checkForUpdate()
{
    printf("Update: Checking for app update...\n");
    std::string s;
    if (!HTTP::GetAsync(_ioService, _url, _headers,
        [this](const int code, const std::string& content, const HTTP::Headers&)
        {
            if (code == 200)
                releasesResponse(content);
            else
                fprintf(stderr, "Update: server returned code %d\n", code);
        },
        [](...)
        {
            fprintf(stderr, "Update: error getting response\n");
        }
    )) {
        fprintf(stderr, "Update: error starting request\n");
    }
}

void AppUpdater::releasesResponse(const std::string &content)
{
    try {
        std::string altVersionStr;
        std::string versionStr;
        std::vector<gh::api::ReleaseAsset> assets;
        std::string browserUrl;
        std::string altBrowserUrl;
        bool newerVersionExists = false;
        for (const auto& rls: gh::api::Releases(content)) {
            if ((rls.tagName()[0] != 'v' && rls.tagName()[0] != 'V') || sanitize_print(rls.tagName()) != rls.tagName())
                continue; // unexpected tag
            versionStr = rls.tagName().substr(1);
            const auto v = Version{versionStr};
            if (!isNewer(v) || (rls.prerelease() && !_includePrerelease && !isSameBaseVersion(v))) {
                // not an update
                versionStr.clear();
                continue;
            }
            browserUrl = rls.htmlUrl();
            newerVersionExists = true;
            for (const auto& asset: rls.assets()) {
                // collect signed assets
                if (asset.nameEndsWithCI(".minisig"))
                    continue;
                if (!rls.hasAssetUrl(asset.browserDownloadUrl() + ".minisig"))
                    continue;
                assets.push_back(asset);
            }
            if (assets.empty()) {
                versionStr.clear();
                continue;
            }
            fs::path pubKeyDir = fs::app_path() / "key";
            if (!fs::is_directory(pubKeyDir))
                pubKeyDir = fs::current_path() / "key";
            bool foundKey = false;
            for (auto& entry: fs::directory_iterator(pubKeyDir)) {
                if (entry.is_regular_file()) {
                    if (rls.bodyContains(entry.path().filename())) {
                        foundKey = true;
                        break; // found an installable update
                    }
                    std::string keyData;
                    if (readFile(entry.path(), keyData)) {
                        const auto p1 = keyData.find('\n');
                        if (p1 != std::string::npos) {
                            const auto p2 = keyData.find_first_of("\r\n", p1 + 1);
                            keyData = keyData.substr(
                                p1 + 1,
                                p2 == std::string::npos ? std::string::npos : (p2 - p1 - 1)
                            );
                            if (rls.bodyContains(keyData)) {
                                foundKey = true;
                                break; // found an installable update
                            }
                        }
                    }
                }
            }
            if (foundKey)
                break; // found an update
            // remember update that can be downloaded, but can't be verified/auto-installed
            if (altVersionStr.empty()) {
                altVersionStr = std::move(versionStr);
                versionStr.clear();
                altBrowserUrl = std::move(browserUrl);
            }
        }
        if (!versionStr.empty())
            updateAvailable(versionStr, browserUrl, assets);
        else if (!altVersionStr.empty()) // TODO: also check if primary pub key expired
            updateAvailable(altVersionStr, altBrowserUrl, {});
        else if (newerVersionExists)
            printf("Update: no candidate for installation\n");
        else
            printf("Update: already up to date\n");
    } catch (...) {
        fprintf(stderr, "Update: error parsing json\n");
    }
}

void AppUpdater::updateAvailable(const std::string& version, const std::string& url,
    const std::vector<gh::api::ReleaseAsset>& assets)
{
    std::string ignoreData;
    json ignore;
    if (readFile(_ignoreVersionsPath, ignoreData)) {
        ignore = parse_jsonc(ignoreData);
        if (ignore.is_array()) {
            if (std::find(ignore.begin(), ignore.end(), version) != ignore.end()) {
                printf("Update: %s is in ignore list\n", version.c_str());
                return;
            }
        } else {
            ignore = json::array();
        }
    }

#if defined _WIN32
    constexpr std::string_view updatePattern = "_win64.zip";
#else
    // currently only implemented for Windows
    // constexpr std::string_view updatePattern = "_ubuntu-22-04-x86_64.tar.xz"; // for testing
    constexpr std::string_view updatePattern; // none
#endif

    const auto autoUpdateIt = std::find_if(assets.begin(), assets.end(), [updatePattern](const auto& asset) {
        const auto& download = asset.browserDownloadUrl();
        return !updatePattern.empty() && download.length() > updatePattern.length()
            && download.rfind(updatePattern, download.length() - updatePattern.length()) != std::string::npos;
    });
    const bool hasAutoUpdate = autoUpdateIt != assets.end();

#ifdef _WIN32
    auto updater = hasAutoUpdate ? (fs::app_path() / "PopUpdater.exe") : "";
    {
        const auto newUpdater = fs::app_path() / "PopUpdater.new.exe";
        const bool updaterSupported = fs::is_regular_file(updater);
        const bool newUpdaterSupported = fs::is_regular_file(newUpdater);
#else
    auto updater = hasAutoUpdate ? (fs::app_path() / "PopUpdater") : "";
    if (hasAutoUpdate) {
        const auto newUpdater = fs::app_path() / "PopUpdater.new";
        const bool updaterSupported = 0 == access(updater.c_str(), X_OK);
        const bool newUpdaterSupported = 0 == access(newUpdater.c_str(), X_OK);
#endif
        if (newUpdaterSupported && updaterSupported && fs::last_write_time(updater) > fs::last_write_time(newUpdater)) {
            // updater.new older than updater -> use updater, try to remove updater.new
            fs::error_code ec;
            fs::remove(newUpdater, ec);
        } else if (newUpdaterSupported) {
            // updater.new newer than updater -> rename updater.new to updater
            fs::error_code ec;
            fs::rename(newUpdater, updater, ec);
            if (ec) {
                // failed to rename updater.new -> updater, use updater.new directly
                updater = newUpdater;
            }
        } else if (!updaterSupported) {
            // no updater supported
            printf("Update: no updater\n");
            updater.clear();
        }
    }

    bool userWritable = false;
    if (!updater.empty()) {
        if (writeFile(fs::app_path() / ".check", "check")) {
            userWritable = true;
            fs::error_code ec;
            fs::remove(fs::app_path() / ".check", ec);
        }
    }

    std::string msg;
    if (assets.empty()) {
        msg = "Update to PopTracker " + version + " available, that can't be verified. Download?";
#ifdef _WIN32
    } else if (!updater.empty()) {
#else
    } else if (userWritable && !updater.empty()) {
        // we currently only support elevating privileges on Windows
#endif
        msg = "Update to PopTracker " + version + " available. Download and install?";
    } else {
        msg = "Update to PopTracker " + version + " available. Download?";
    }
    if (Dlg::MsgBox("PopTracker", msg, Dlg::Buttons::YesNo, Dlg::Icon::Question) == Dlg::Result::Yes)
    {
        if (!updater.empty()) {
            installUpdate(
                autoUpdateIt->browserDownloadUrl(),
                updater,
                !userWritable,
                autoUpdateIt->updatedAt(),
                url);
            return;
        }
        openWebsite(url);
    }
    else {
        msg = "Skip version " + version + "?";
        if (Dlg::MsgBox("PopTracker", msg, Dlg::Buttons::YesNo, Dlg::Icon::Question) == Dlg::Result::Yes)
        {
            printf("Update: ignoring %s\n", version.c_str());
            ignore.push_back(version);
            writeFile(_ignoreVersionsPath, ignore.dump(0));
        }
    }
}

void AppUpdater::installUpdate(const std::string& url, const fs::path& updater, const bool asAdmin,
    const uint64_t timestamp, const std::string& browserUrl)
{
    fs::path path;
    int fd = -1;
    std::string ext;
    const auto p = url.find_last_of('.');
    if (p == std::string::npos) {
        ext = ".zip";
    } else {
        ext = sanitize_filename(url.substr(p));
    }

    for (int i=0; i<5; i++) {
        std::string name = GetRandomName(ext);
        path = _cachedir / name;
#ifdef _WIN32
        fd = _wopen(path.c_str(), O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL, 0600);
#else
        fd = open(path.c_str(), O_WRONLY | O_CLOEXEC | O_CREAT | O_EXCL, 0600);
#endif
        if (fd>=0)
            break;
    }
    if (fd < 0) {
        onUpdateFailed.emit(this, url, "Could not create temp file!");
        return;
    }
    auto sigPath = path;
    sigPath += ".minisig";
    const auto sigUrl = url + ".minisig";
    close(fd);
    getFile(sigUrl, sigPath, [=](const bool sigOk, const std::string& sigMsg) {
        if (sigOk) {
            getFile(url, path, [=](const bool ok, const std::string& msg){
                if (ok) {
                    fs::error_code ec;
                    const auto total = fs::file_size(path, ec);
                    if (!ec) {
                        printf("Update: download complete: %zu\n", total);
                        onUpdateProgress.emit(this, url, static_cast<int>(total), static_cast<int>(total));
                    }
#ifdef _WIN32
                    const auto op = asAdmin ? L"runas" : L"open";
                    auto repoSize = MultiByteToWideChar(CP_UTF8, 0, _repo.c_str(), _repo.size(), nullptr, 0);
                    std::wstring wRepo(repoSize, '\0');
                    if (MultiByteToWideChar(CP_UTF8, 0, _repo.c_str(), _repo.size(), wRepo.data(), repoSize) < 1
                            && _repo.size() > 0) {
                        throw std::runtime_error("Could not convert string");
                    }
                    auto pid = GetCurrentProcessId();
                    auto param = fmt::format(LR"(-a "{}" -t {} -k {} -s poptracker.exe "{}" "{}" "{}")",
                        wRepo, timestamp, pid,
                        fs::app_path().c_str(), path.c_str(), sigPath.c_str());
                    wprintf(L"Update: starting %S %S %S\n", op, updater.c_str(), param.c_str());
                    SHELLEXECUTEINFOW execInfo = {};
                    // NOTE: Our main loop may not continue long enough for ASYNC
                    // NOTE: Zone checks is checking mark of the web, which could fail depending on UAC settings
                    execInfo.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOZONECHECKS;
                    execInfo.cbSize = sizeof(execInfo);
                    execInfo.lpVerb = op;
                    execInfo.lpFile = updater.c_str();
                    execInfo.lpParameters = param.c_str();
                    execInfo.nShow = SW_SHOWDEFAULT;
                    const auto res = ShellExecuteExW(&execInfo);
                    if (res) {
                        onInstallStarted.emit(this);
                        return;
                    } else {
                        const auto lastError = GetLastError();
                        onUpdateFailed.emit(this, url, fmt::format("Could not start updater: ({}) {}",
                            lastError, GetWindowsErrorString(lastError)));
                    }
#else
                    onUpdateFailed.emit(this, url, "Auto-updating not implemented for this platform!");
                    (void)updater;
                    (void)asAdmin;
                    (void)timestamp;
#endif
                    if (!browserUrl.empty()) {
                        if (Dlg::MsgBox(
                            "PopTracker",
                            "Open website instead?",
                            Dlg::Buttons::YesNo,
                            Dlg::Icon::Question) == Dlg::Result::Yes)
                        {
                            openWebsite(browserUrl);
                        }
                    }
                } else {
                    onUpdateFailed.emit(this, url, msg);
                }
            }, [this, url](const int transferred, const int total) {
                onUpdateProgress.emit(this, url, transferred, total);
            }, 5);
        } else {
            onUpdateFailed.emit(this, url, sigMsg);
        }
    }, [this, url](const int transferred, const int total) {
        onUpdateProgress.emit(this, url, transferred, total);
    }, 5);
}

void AppUpdater::getFile(const std::string& url, const fs::path& dest,
        const std::function<void(bool, const std::string&)>& cb,
        const std::function<void(int, int)>& progress,
        const int followRedirect) const
{
#ifdef _WIN32
    FILE* f = _wfopen(dest.c_str(), L"wb");
#else
    FILE* f = fopen(dest.c_str(), "wb");
#endif
    if (!f) cb(false, strerror(errno));
    HTTP::GetAsync(*_asio, url, _httpDefaultHeaders, [=](int code, const std::string& response, const HTTP::Headers&){
        fclose(f);
        if (code == HTTP::REDIRECT && followRedirect>0) {
            getFile(response, dest, cb, progress, followRedirect-1);
        } else if (code == HTTP::REDIRECT) {
            cb(false, "Too many redirects");
        } else {
            cb(code==HTTP::OK, response);
        }
    }, [=]() {
        fclose(f);
        cb(false, "");
    }, f, progress);
}

bool AppUpdater::isNewer(const Version& v)
{
    return v > PopTracker::VERSION;
}

bool AppUpdater::isSameBaseVersion(const Version& v)
{
    const Version& v2 = PopTracker::VERSION;
    return v.Major == v2.Major && v.Minor == v2.Minor && v.Revision == v2.Revision;
}

} // pop
