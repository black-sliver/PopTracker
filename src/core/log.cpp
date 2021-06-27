#include "log.h"


std::string Log::_logFile = "";
int Log::_oldOut = -1;
int Log::_oldErr = -1;
int Log::_newOut = -1;


#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)) || defined(__MINGW32__) || defined(__GNUC__)
#include <unistd.h>
#else
#include <io.h>
#endif

#include <fcntl.h>
#include <stdio.h>

bool Log::RedirectStdOut(const std::string& file, bool truncate) {
    fflush(stdout);
    fflush(stderr);
    _logFile = file;
    UnredirectStdOut();
    _newOut = open(_logFile.c_str(), O_RDWR|O_CREAT|O_APPEND|(truncate ? O_TRUNC : 0), 0655);
    if (_newOut == -1) {
        fprintf(stderr, "Could not open %s for logging!\n",
                _logFile.c_str());
        return false;
    }
    _oldErr = dup(fileno(stderr));
    _oldOut = dup(fileno(stdout));

    if (dup2(_newOut, fileno(stdout)) == -1) {
        fprintf(stderr, "Could not redirect stdout!\n");
        close(_newOut);
        close(_oldErr);
        close(_oldOut);
        _newOut = -1;
        _oldErr = -1;
        _oldOut = -1;
        return false;
    }
    if (dup2(_newOut, fileno(stderr)) == -1) {
        printf("Could not redirect stderr! Log may be incomplete!\n");
    }
    fflush(stdout);
    fflush(stderr);
    return true;
}
void Log::UnredirectStdOut() {
    if (_newOut == -1) return;
    fflush(stdout);
    fflush(stderr);
    close(_newOut);
    _newOut = -1;

    dup2(_oldOut, fileno(stdout));
    dup2(_oldErr, fileno(stderr));

    close(_oldOut);
    close(_oldErr);
}