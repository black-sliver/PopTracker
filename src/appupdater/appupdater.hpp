#pragma once

#include <asio.hpp>
#include <list>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include "../core/fs.h"
#include "../core/signal.h"
#include "../core/version.h"
#include "../gh/api/releaseasset.hpp"
#include "../http/httpcache.h"


namespace pop {
    class AppUpdater final : HTTPCache {
    public:
        using json = nlohmann::json;

        AppUpdater(asio::io_service& ioService, const std::list<std::string>& headers, std::string_view owner,
            std::string_view repo, const bool includePrereleases, fs::path ignoreVersionsPath, const fs::path& cacheDir)
            : HTTPCache(&ioService, cacheDir / "update-cache.json", cacheDir / "update-cache", headers),
              _ioService(ioService), _headers(headers), _includePrerelease(includePrereleases),
              _ignoreVersionsPath(std::move(ignoreVersionsPath))
        {
            _repo = fmt::format("{}/{}", owner, repo);
            _url = fmt::format("https://api.github.com/repos/{}/releases?per_page=8", _repo);
        }

        /// Start checking for app updates
        void checkForUpdate();

        Signal<const std::string&, int, int> onUpdateProgress;
        Signal<> onInstallStarted;
        Signal<const std::string&, const std::string&> onUpdateFailed;

    private:
        /// Handle OK response of app update check
        void releasesResponse(const std::string &content);
        /// Notification that an app update is available
        void updateAvailable(const std::string& version, const std::string& url,
            const std::vector<gh::api::ReleaseAsset>& assets);
        /// Download and install an app update
        void installUpdate(const std::string& url, const fs::path& updater, bool asAdmin, uint64_t timestamp,
            const std::string& browserUrl="");

        /// Download a file
        void getFile(const std::string& url, const fs::path& dest,
                const std::function<void(bool, const std::string&)>& cb,
                const std::function<void(int, int)>& progress,
                int followRedirect) const;

        /// returns true if v is newer than current PopTracker
        static bool isNewer(const Version& v);
        /// returns true if major, minor and revision of v and current PopTracker is the same
        static bool isSameBaseVersion(const Version &v);

        asio::io_service& _ioService;
        const std::list<std::string>& _headers;
        std::string _repo;
        std::string _url;
        bool _includePrerelease;
        fs::path _ignoreVersionsPath;
    };
} // pop
