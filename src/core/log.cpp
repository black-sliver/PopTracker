#include "log.h"


std::string Log::_logFile = "";
int Log::_oldOut = -1;
int Log::_oldErr = -1;
int Log::_newOut = -1;
int Log::_newErr = -1;


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
    _newErr = dup(_newOut);
    if (_newOut == -1) {
        fprintf(stderr, "Could not open %s for logging!\n",
                _logFile.c_str());
        return false;
    }
    _oldErr = dup(fileno(stderr));
    _oldOut = dup(fileno(stdout));
    
    // if stdout has no underlying handle, use freopen (fileno(stdout) is -2 on windows)
    if (_oldOut < 0) {
        freopen(_logFile.c_str(), "w", stdout);
        setvbuf(stdout, NULL, _IONBF, 0); // _IOLBF
        close(_newOut);
        _newOut = fileno(stdout);
        if (_newOut < 0) {
            fprintf(stderr, "Could not redirect stdout!\n");
            close(_oldErr);
            _oldErr = -1;
            return false;
        }
    }
    // otherwise have stdout file handle point to file
    else if (dup2(_newOut, fileno(stdout)) == -1) {
        fprintf(stderr, "Could not redirect stdout!\n");
        close(_newOut);
        close(_oldErr);
        close(_oldOut);
        _newOut = -1;
        _oldErr = -1;
        _oldOut = -1;
        return false;
    }

    // if stderr has no underlying handle, use freopen
    if (_oldErr < 0 || _newErr < 0) {
        freopen(_logFile.c_str(), "w", stderr);
        setvbuf(stderr, NULL, _IONBF, 0);
        close(_newErr);
        _newErr = fileno(stderr);
        if (_newErr < 0) {
            printf("Could not redirect stderr! Log may be incomplete!\n");
        }
    }
    // otherwise have stderr file handle point to file
    else if (dup2(_newErr, fileno(stderr)) == -1) {
        printf("Could not redirect stderr! Log may be incomplete!\n");
    }
    
    fflush(stdout);
    fflush(stderr);
    return true;
}
void Log::UnredirectStdOut() {
    if (_newOut == -1 && _newErr == -1) return;
    fflush(stdout);
    fflush(stderr);
    close(_newOut);
    close(_newErr);
    _newOut = -1;
    _newErr = -1;
    
    if (_oldOut >= 0) {
        dup2(_oldOut, fileno(stdout));
        close(_oldOut);
        _oldOut = -1;
    } else {
        // nothing to restore, just close underlying FD
        close(fileno(stdout));
    }
    if (_oldErr >= 0) {
        dup2(_oldErr, fileno(stderr));
        close(_oldErr);
        _oldErr = -1;
    } else {
        // nothing to restore, just close underlying FD
        close(fileno(stderr));
    }
}
