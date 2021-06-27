/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Log.h
 * Author: andy
 *
 * Created on June 27, 2021, 11:15 AM
 */

#ifndef LOG_H
#define LOG_H

#include <string>

class Log final {
private:
    static std::string _logFile;
    static int _oldOut;
    static int _oldErr;
    static int _newOut;
    
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

