#include "version.h"

Version::Version(int ma, int mi, int r, const std::string& e)
        : major(ma), minor(mi), revision(r), extra(e)
{
}

Version::Version(const std::string& s)
{
    char* next = nullptr;
    const char* start = s.c_str();
    if (*start == 'v' || *start == 'V') start++;
    major = (int)strtol(start, &next, 10);
    minor = (next && *next) ? (int)strtol(next+1, &next, 10) : 0;
    revision = (next && *next) ? (int)strtol(next+1, &next, 10) : 0;
    extra = (next && *next) ? next : "";
}

Version::~Version()
{
}

bool Version::operator <(const Version& other) const
{
    if (major < other.major) return true;
    if (major > other.major) return false;
    if (minor < other.minor) return true;
    if (minor > other.minor) return false;
    if (revision < other.revision) return true;
    if (revision > other.revision) return false;
    return false;
}

bool Version::operator >(const Version& other) const
{
    return other < *this;
}