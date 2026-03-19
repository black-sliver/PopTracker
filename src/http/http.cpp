#include "http.h"

std::string HTTP::certFile;
std::set<std::string> HTTP::dntTrustedHosts = {
    "github.com",
    "www.github.com",
    "api.github.com",
    "raw.githubusercontent.com",
};
