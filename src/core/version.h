#ifndef _CORE_VERSION_H
#define _CORE_VERSION_H

#include <string>

class Version final {
public:
    Version(int major, int minor, int revision, const std::string& extra="");
    Version(const std::string& s);
    virtual ~Version();
    
    int Major;
    int Minor;
    int Revision;
    std::string Extra;
    
    bool operator<(const Version& other) const;
    bool operator>(const Version& other) const;
};

#endif /* _CORE_VERSION_H */

