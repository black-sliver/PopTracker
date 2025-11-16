#include "appupdater.hpp"
#include <cstdio> // TODO: move to fmt::print
#include "../poptracker.h"
#include "../core/jsonutil.h"
#include "../gh/api/releases.hpp"
#include "../http/http.h"
#include "../uilib/dlg.h"


using namespace Ui;

namespace pop {

void AppUpdater::checkForUpdate() const
{
    printf("Checking for update...\n");
    std::string s;
    // .../releases/latest would be better, but does not return pre-releases
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

void AppUpdater::releasesResponse(const std::string &content) const
{
    try {
        std::string altVersionStr;
        std::string versionStr;
        std::vector<std::string> assets;
        std::string browserUrl;
        std::string altBrowserUrl;
        bool newerVersionExists = false;
        for (const auto& rls: gh::api::Releases(content)) {
            if (rls.tagName()[0] != 'v' && rls.tagName()[0] != 'V')
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
                assets.push_back(asset.browserDownloadUrl());
            }
            if (assets.empty()) {
                versionStr.clear();
                continue;
            }
            fs::path pubKeyDir = fs::app_path() / "key";
            if (!fs::is_directory(pubKeyDir))
                pubKeyDir = fs::current_path() / "key";
            for (auto& entry: fs::directory_iterator(pubKeyDir)) {
                if (entry.is_regular_file()) {
                    if (rls.bodyContains(entry.path().filename().c_str())) {
                        break; // found an installable update
                    }
                    std::string keyData;
                    if (readFile(entry.path(), keyData)) {
                        const auto p1 = keyData.find('\n');
                        if (p1 != std::string::npos) {
                            const auto p2 = keyData.find_first_of("\r\n");
                            keyData = keyData.substr(
                                p1,
                                p2 == std::string::npos ? std::string::npos : p2 - p1
                            );
                            if (rls.bodyContains(keyData)) {
                                break; // found an installable update
                            }
                        }
                    }
                }
            }
            // remember update that can be downloaded, but can't be verified/auto-installed
            altVersionStr = versionStr;
            altBrowserUrl = browserUrl;
            break; // found an update
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
    const std::vector<std::string>& assets) const
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

    std::string msg;
    if (assets.empty()) {
        msg = "Update to PopTracker " + version + " available, that can't be verified. Download?";
    } else {
        msg = "Update to PopTracker " + version + " available. Download?";
    }
    if (Dlg::MsgBox("PopTracker", msg, Dlg::Buttons::YesNo, Dlg::Icon::Question) == Dlg::Result::Yes)
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
            msg = "Could not launch browser!\n";
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
