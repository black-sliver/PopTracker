#include "http.h"

std::string HTTP::certfile;
std::set<std::string> HTTP::dnt_trusted_hosts = {
    "github.com",
    "www.github.com",
    "api.github.com",
    "raw.githubusercontent.com",
};
