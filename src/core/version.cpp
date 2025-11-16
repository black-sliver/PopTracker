#include "version.h"
#include <algorithm>
#include <utility>


Version::Version(const int major, const int minor, const int revision, std::string  extra)
        : Major(major), Minor(minor), Revision(revision), Extra(std::move(extra))
{
    sanitize();
}

Version::Version(const int major, const int minor, const int revision, const int extra)
        : Major(major), Minor(minor), Revision(revision)
{
    if (extra)
        Extra = "." + std::to_string(extra);
}

Version::Version(const std::string& s)
{
    char* next = nullptr;
    const char* start = s.c_str();
    if (*start == 'v' || *start == 'V') start++;
    Major = static_cast<int>(strtol(start, &next, 10));
    Minor = (next && *next) ? static_cast<int>(strtol(next + 1, &next, 10)) : 0;
    Revision = (next && *next) ? static_cast<int>(strtol(next + 1, &next, 10)) : 0;
    Extra = (next && *next) ? next : "";
    sanitize();
}

Version::Version()
    : Major(0), Minor(0), Revision(0)
{
}

bool Version::operator <(const Version& other) const
{
    if (Major < other.Major) return true;
    if (Major > other.Major) return false;
    if (Minor < other.Minor) return true;
    if (Minor > other.Minor) return false;
    if (Revision < other.Revision) return true;
    if (Revision > other.Revision) return false;
    if ((!Extra.empty() && Extra[0] == '.') || (!other.Extra.empty() && other.Extra[0] == '.')) {
        char* next = nullptr;
        const auto numericExtra = Extra.empty() ? 0 : strtol(Extra.c_str() + 1, &next, 10);
        char* otherNext = nullptr;
        const auto otherNumericExtra = other.Extra.empty() ? 0 : strtol(other.Extra.c_str() + 1, &otherNext, 10);
        if ((next == nullptr || *next == 0) && (otherNext == nullptr || *otherNext == 0)) {
            // not semver; only use result if BOTH are "a.b.c[.d]"
            return numericExtra < otherNumericExtra;
        }
    }
    // remove build identifier (+...) for comparison of pre-release identifier (-...)
    const auto plusPos = Extra.find('+');
    const auto otherPlusPos = other.Extra.find('+');
    const auto relevantExtraLen = plusPos == std::string::npos ? Extra.length() : plusPos;
    const auto otherRelevantExtraLen = otherPlusPos == std::string::npos ? other.Extra.length() : otherPlusPos;
    const std::string_view relevantExtra(Extra.c_str(), relevantExtraLen);
    const std::string_view otherRelevantExtra(other.Extra.c_str(), otherRelevantExtraLen);
    if (relevantExtra.empty())
        return false;
    if (otherRelevantExtra.empty())
        return true;
    if (relevantExtra.compare(otherRelevantExtra) < 0)
        return true;
    return false;
}

bool Version::operator >(const Version& other) const
{
    return other < *this;
}

bool Version::operator >=(const Version& other) const
{
    return !(*this < other);
}

std::string Version::to_string() const
{
    return std::to_string(Major) + "." + std::to_string(Minor) + "." + std::to_string(Revision) + Extra;
}

void Version::sanitize()
{
    // we are a bit more lax than semver here, but this should still allow to compare
    auto invalid = [&](const char c) { return c < '(' || c > '~' || (c > '_' && c < 'a'); };
    Extra.erase(std::remove_if(Extra.begin(), Extra.end(), invalid), Extra.end());
    if (!Extra.empty() && Extra[0] != '-' && Extra[0] != '+' && Extra[0] != '.')
        Extra = "-" + Extra;
}
