#ifndef _CORE_ZIP_H
#define _CORE_ZIP_H

#include <miniz.h>
#include <string>
#include <vector>
#include <utility>

#include "fs.h"

class Zip final
{
public:
    enum class EntryType {
        FILE,
        DIR
    };
    
    Zip(const fs::path& filename);
    virtual ~Zip();
    
    void setDir(const std::string&);
    std::vector< std::pair<EntryType,std::string> > list(bool recursive=false);
    bool hasFile(const std::string& name);
    bool readFile(const std::string& name, std::string& out);
    bool readFile(const std::string& name, std::string& out, std::string& err);
    
private:
    enum class Slashes : unsigned {
        UNKNOWN=0,
        FORWARD=1,
        BACKWARD=2,
        MIXED=3,
    };
    friend Zip::Slashes  operator| (Zip::Slashes,  Zip::Slashes);
    friend Zip::Slashes& operator|=(Zip::Slashes&, Zip::Slashes);
    mz_zip_archive _zip;
    bool _valid;
    Slashes _slashes;
    std::string _dir;    
};

inline Zip::Slashes operator|(Zip::Slashes a, Zip::Slashes b)
{
    return static_cast<Zip::Slashes>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
}
inline Zip::Slashes& operator|=(Zip::Slashes& a, Zip::Slashes b)
{
    return a = a|b;
}

#endif
