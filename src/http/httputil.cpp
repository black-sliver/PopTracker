#include "httputil.hpp"

#include <algorithm>
#include <cstdio>

#include "../uilib/dlg.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#endif

using namespace Ui;

namespace HttpUtil {

void openWebsite(const std::string& url)
{
#if defined _WIN32 || defined WIN32
    ShellExecuteA(nullptr, "open", url.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#else
    const auto pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Update: fork failed\n");
    }
    else if (pid == 0) {
#if defined __APPLE__ || defined MACOS
        const char* exe = "open";
#else
        const auto exe = "xdg-open";
#endif
        execlp(exe, exe, url.c_str(), nullptr);
        std::string msg = "Could not launch browser!\n";
        msg += exe;
        msg += ": ";
        msg += strerror(errno);
        fprintf(stderr, "Update: %s\n", msg.c_str());
        Dlg::MsgBox("PopTracker", msg,
                    Dlg::Buttons::OK, Dlg::Icon::Error);
        exit(0);
    }
#endif
}

std::string sanitizeUrl(std::string s)
{
    const auto exclude = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;=";
    auto sanitize = [&](const char c) { return !strchr(exclude, c); };
    s.erase(std::remove_if(s.begin(), s.end(), sanitize), s.end());
    return s;
}

}
