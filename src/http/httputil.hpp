#ifndef _HTTP_HTTPUTIL_H
#define _HTTP_HTTPUTIL_H

#include <string>

namespace HttpUtil {

void openWebsite(const std::string& url);
std::string sanitizeUrl(std::string s);

}

#endif // _HTTP_HTTPUTIL_H