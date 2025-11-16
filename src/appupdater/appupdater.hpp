#pragma once

#include <asio.hpp>
#include <list>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include "../core/fs.h"
#include "../core/version.h"


namespace pop {
    class AppUpdater {
    public:
        using json = nlohmann::json;

        AppUpdater(asio::io_service& ioService, const std::list<std::string>& headers, std::string_view owner,
            std::string_view repo, const bool includePrereleases, fs::path ignoreVersionsPath)
            : _ioService(ioService), _headers(headers), _includePrerelease(includePrereleases),
              _ignoreVersionsPath(std::move(ignoreVersionsPath))
        {
            _url = fmt::format("https://api.github.com/repos/{}/{}/releases?per_page=8", owner, repo);
        }

        /// Start checking for app updates
        void checkForUpdate() const;

    private:
        /// Handle OK response of app update check
        void releasesResponse(const std::string &content) const;
        /// Notification that an app update is available
        void updateAvailable(const std::string& version, const std::string& url,
            const std::vector<std::string>& assets) const;

        /// returns true if v is newer than current PopTracker
        static bool isNewer(const Version& v);
        /// returns true if major, minor and revision of v and current PopTracker is the same
        static bool isSameBaseVersion(const Version &v);

        asio::io_service& _ioService;
        const std::list<std::string>& _headers;
        std::string _url;
        bool _includePrerelease;
        fs::path _ignoreVersionsPath;
    };
} // pop
