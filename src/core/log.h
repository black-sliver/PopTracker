#ifndef LOG_H
#define LOG_H

#include <string>

class Log final {
private:
    static std::string _logFile;
    static int _oldOut;
    static int _oldErr;
    static int _newOut;
    static int _newErr;
    
public:
    static std::string getFile() { return _logFile; }
    static bool RedirectStdOut(const std::string& file, bool truncate=true);
    static void UnredirectStdOut();
    
    
private:
    Log(){};
    Log(const Log& orig) = delete;
public:
    virtual ~Log(){};
private:

};

#endif /* LOG_H */

