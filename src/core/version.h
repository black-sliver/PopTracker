#ifndef _CORE_VERSION_H
#define _CORE_VERSION_H

#include <string>

class Version final {
public:
    Version(int major, int minor, int revision, const std::string& extra="");
    Version(const std::string& s);
    virtual ~Version();
    
    int major;
    int minor;
    int revision;
    std::string extra;
    
    bool operator<(const Version& other) const;
    bool operator>(const Version& other) const;
};

#endif /* _CORE_VERSION_H */

