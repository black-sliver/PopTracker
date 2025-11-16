#pragma once

#include <string>


/// Version parser and comparison utility supporting both semver and a[.b[.c[.d]]] notation
class Version final {
public:
    Version(int major, int minor, int revision, std::string extra = "");
    Version(int major, int minor, int revision, int extra);
    explicit Version(const std::string& s);
    Version();
    
    int Major;
    int Minor;
    int Revision;
    std::string Extra;
    
    bool operator<(const Version& other) const;
    bool operator>(const Version& other) const;
    bool operator>=(const Version& other) const;

    std::string to_string() const;

private:
    void sanitize();
};
