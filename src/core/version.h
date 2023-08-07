#ifndef _CORE_VERSION_H
#define _CORE_VERSION_H

#include <string>

class Version final {
public:
    Version(int major, int minor, int revision, const std::string& extra="");
    Version(int major, int minor, int revision, int extra);
    Version(const std::string& s);
    Version();
    virtual ~Version();
    
    int Major;
    int Minor;
    int Revision;
    std::string Extra;
    
    bool operator<(const Version& other) const;
    bool operator>(const Version& other) const;

    std::string to_string() const;
};

#endif /* _CORE_VERSION_H */

