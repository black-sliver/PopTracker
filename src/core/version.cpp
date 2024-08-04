#include "version.h"
#include <algorithm>


Version::Version(int ma, int mi, int r, const std::string& e)
        : Major(ma), Minor(mi), Revision(r), Extra(e)
{
    sanitize();
}

Version::Version(int ma, int mi, int r, int e)
        : Major(ma), Minor(mi), Revision(r)
{
    if (e)
        Extra = "." + std::to_string(e);
}

Version::Version(const std::string& s)
{
    char* next = nullptr;
    const char* start = s.c_str();
    if (*start == 'v' || *start == 'V') start++;
    Major = (int)strtol(start, &next, 10);
    Minor = (next && *next) ? (int)strtol(next+1, &next, 10) : 0;
    Revision = (next && *next) ? (int)strtol(next+1, &next, 10) : 0;
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
        auto numericExtra = Extra.empty() ? 0 : strtol(Extra.c_str() + 1, &next, 10);
        char* otherNext = nullptr;
        auto otherNumericExtra = other.Extra.empty() ? 0 : strtol(other.Extra.c_str() + 1, &otherNext, 10);
        if ((next == nullptr || *next == 0) && (otherNext == nullptr || *otherNext == 0)) {
            // not semver; only use result if BOTH are "a.b.c[.d]"
            return numericExtra < otherNumericExtra;
        }
    }
    auto plusPos = Extra.find('+');
    auto otherPlusPos = other.Extra.find('+');
    auto npos = std::string::npos;
    std::string_view cutExtra(Extra.c_str(), plusPos == npos ? Extra.length() : plusPos);
    std::string_view otherCutExtra(other.Extra.c_str(), otherPlusPos == npos ? other.Extra.length() : otherPlusPos);
    if (cutExtra.empty())
        return false;
    if (otherCutExtra.empty())
        return true;
    if (cutExtra.compare(otherCutExtra) < 0)
        return true;
    return false;
}

bool Version::operator >(const Version& other) const
{
    return other < *this;
}

std::string Version::to_string() const
{
    return std::to_string(Major) + "." + std::to_string(Minor) + "." + std::to_string(Revision) + Extra;
}

void Version::sanitize()
{
    // we are a bit more lax than semver here, but this should still allow to compare
    auto invalid = [&](char c) { return c < '(' || c > '~' || (c > '_' && c < 'a'); };
    Extra.erase(std::remove_if(Extra.begin(), Extra.end(), invalid), Extra.end());
    if (!Extra.empty() && Extra[0] != '-' && Extra[0] != '+' && Extra[0] != '.')
        Extra = "-" + Extra;
}
