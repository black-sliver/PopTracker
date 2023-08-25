#include "version.h"

Version::Version(int ma, int mi, int r, const std::string& e)
        : Major(ma), Minor(mi), Revision(r), Extra(e)
{
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
}

Version::Version()
    : Major(0), Minor(0), Revision(0), Extra("")
{
}

Version::~Version()
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
    return false;
}

bool Version::operator >(const Version& other) const
{
    return other < *this;
}

std::string Version::to_string() const
{
    if (Extra.empty())
        return std::to_string(Major) + "." + std::to_string(Minor) + "." + std::to_string(Revision);
    return std::to_string(Major) + "." + std::to_string(Minor) + "." + std::to_string(Revision) + "-" + Extra;
}