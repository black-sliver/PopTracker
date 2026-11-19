#ifndef _PACKMANAGER_PACKLIST_H
#define _PACKMANAGER_PACKLIST_H

#include <string>

/// Order of entries determines priority, if two lists provide the same pack UID, the earlier one is shown.
struct PackListEntry {
    std::string name;
    std::string url;
    bool enabled = false;
    bool builtin = false; // community remote list, always listed first, disabled by default
};

namespace PackLists {
inline constexpr char COMMUNITY_NAME[] = "PopTracker Community Packlist";
inline constexpr char COMMUNITY_DESCRIPTION[] = "Remote Packlist maintained by PopTracker community members and pack developers.";
inline constexpr char COMMUNITY_HOMEPAGE[] = "https://github.com/poptracker-community/community-packlist";
inline constexpr char COMMUNITY_URL[] =
    "https://raw.githubusercontent.com/poptracker-community/community-packlist/main/community-packs.json";
} // namespace PackLists

#endif /* _PACKMANAGER_PACKLIST_H */
