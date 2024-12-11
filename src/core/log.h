#ifndef LOG_H
#define LOG_H

#include <string>
#include "fs.h"

class Log final {
private:
    static fs::path _logFile;
    static int _oldOut;
    static int _oldErr;
    static int _newOut;
    static int _newErr;
    
public:
    static const fs::path& getFile() { return _logFile; }
    static bool RedirectStdOut(const fs::path& file, bool truncate=true);
    static void UnredirectStdOut();
    
    
private:
    Log(){};
    Log(const Log& orig) = delete;
public:
    virtual ~Log(){};
private:

};

#endif /* LOG_H */

