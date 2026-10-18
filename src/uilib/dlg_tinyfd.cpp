#include "dlg.h"

#include <cstdio>
#include <list>
#include <mutex>
#include <string>
#include <tinyfiledialogs.h>
#include "../core/fs.h"

#include <tinyfiledialogs.cpp>

// NOTE: tinyfd_inputBox does not work in wine

#if defined __WIN32__
#warning "Use dlg_win32 on Windows!!"
#elif defined __APPLE__
#warning "Use dlg_cocoa on macOS!!"
#endif


namespace Ui {

static std::mutex _mutex;
static bool _hasGUI = false;
static bool _hasGUISet = false;

bool Dlg::InputBox(const std::string& title, const std::string& message, const std::string& dflt, std::string& result, bool password)
{
    std::lock_guard lock(_mutex);
    // NOTE: some of tinyfd's UI providers do not like `'` and/or `\` in the default. The output is fine though.
    // TODO: switch to a different dialog system for Linux
    const bool defaultInvalid = dflt.find_first_of("'\\") != std::string::npos;
    const char* res = tinyfd_inputBox(title.c_str(), message.c_str(),
        password ? nullptr : defaultInvalid ? "" : dflt.c_str());
    if (res)
        result = res;
    return !!res;
}

Dlg::Result Dlg::MsgBox(const std::string& title, const std::string& message, Dlg::Buttons btns, Dlg::Icon icon, Dlg::Result dflt)
{
    const char* tfdicons[] = { "", "info", "warning", "error", "question" };
    const char* tfdicon = tfdicons[(int)icon];
    int res; 
    switch (btns) {
        case Dlg::Buttons::OK:
            tinyfd_messageBox(title.c_str(), message.c_str(), "ok",  tfdicon, 0);
            return Dlg::Result::OK;
        case Dlg::Buttons::OKCancel:
            res = tinyfd_messageBox(title.c_str(), message.c_str(),  "okcancel",  tfdicon, dflt==Dlg::Result::OK ? 1 : 0);
            return res ? Dlg::Result::OK : Dlg::Result::Cancel;
        case Dlg::Buttons::YesNo:
            res = tinyfd_messageBox(title.c_str(), message.c_str(), "yesno", tfdicon, (dflt==Dlg::Result::Yes||dflt==Dlg::Result::OK) ? 1 : 0);
            return res ? Dlg::Result::Yes : Dlg::Result::No;
        case Dlg::Buttons::YesNoCancel:
            res = tinyfd_messageBox(title.c_str(), message.c_str(), "yesnocancel", tfdicon, dflt==Dlg::Result::No ? 2 : dflt==Dlg::Result::Yes ? 1 : 0);
            return res == 2 ? Dlg::Result::No : res ? Dlg::Result::Yes : Dlg::Result::Cancel;
    }
    return Dlg::Result::Cancel;
}

bool Dlg::OpenFile(const std::string& title, const fs::path& dflt, const std::list<FileType>& types, fs::path& out, bool multi)
{
    std::lock_guard<std::mutex> lock(_mutex);
    const char* filename;
    if (types.empty()) {
        filename = tinyfd_openFileDialog(title.c_str(), dflt.c_str(), 0,
                nullptr, nullptr, multi);
    } else {
        const auto& type = types.front(); // tinyfd only supports one
        auto patterns = new const char*[type.patterns.size()];
        int i=0;
#ifdef __APPLE__
        // macos uses types instead of patterns, fix up the difference here
        for (const auto& pat: type.patterns) {
            if (strcasestr(pat.c_str(), ".json"))
                patterns[i++] = "*.public.json"; // UTType json is "public.json"
            else if (strcasestr(pat.c_str(), ".zip"))
                patterns[i++] = "*.public.zip"; // UTType zip is "public.zip"
            else
                patterns[i++] = pat.c_str();
        }
#else
        for (const auto& pat: type.patterns)
            patterns[i++] = pat.c_str();
#endif
        filename = tinyfd_openFileDialog(title.c_str(), dflt.c_str(), i,
                patterns, type.name.c_str(), multi);
        delete[] patterns;
    }
    if (!filename) return false;
    out = filename;
    return true;
}

bool Dlg::SaveFile(const std::string& title, const fs::path& dflt, const std::list<FileType>& types, fs::path& out)
{
    std::lock_guard<std::mutex> lock(_mutex);
    const char* filename;
    if (types.empty()) {
        filename = tinyfd_saveFileDialog(title.c_str(), dflt.c_str(), 0,
                nullptr, nullptr);
    } else {
        const auto& type = types.front(); // tinyfd only supports one
        auto patterns = new const char*[type.patterns.size()];
        int i=0;
#ifdef __APPLE__
        // see Dlg::OpenFile
        for (const auto& pat: type.patterns) {
            if (strcasestr(pat.c_str(), ".json"))
                patterns[i++] = "*.public.json";
            else if (strcasestr(pat.c_str(), ".zip"))
                patterns[i++] = "*.public.zip";
            else
                patterns[i++] = pat.c_str();
        }
#else
        for (const auto& pat: type.patterns)
            patterns[i++] = pat.c_str();
#endif
        filename = tinyfd_saveFileDialog(title.c_str(), dflt.c_str(), i,
                patterns, type.name.c_str());
        delete[] patterns;
    }
    if (!filename) return false;
    out = filename;
    return true;
}

static int tryRun(const char* cmd)
{
    FILE *p = popen(cmd, "r");
    if (!p)
        return -1;
    return pclose(p);
}

bool Dlg::hasGUI()
{
    if (_hasGUISet)
        return _hasGUI;
#if defined __WIN32__ || defined __APPLE__
    // assume yes for windows and macOS
    _hasGUI = true;
#else
    _hasGUI = tryRun("which zenity &>/dev/null") == 0 ||
            tryRun("which kdialog &>/dev/null") == 0 ||
            tryRun("which matedialog &>/dev/null") == 0 ||
            tryRun("which qarma &>/dev/null") == 0 ||
            tryRun("which xdialog &>/dev/null") == 0 ||
            (tryRun("which python3 &>/dev/null") == 0 && tryRun("python3 -c \"import tkinter\" &>/dev/null") == 0) ||
            (tryRun("which python2 &>/dev/null") == 0 && tryRun("python2 -c \"import Tkinter\" &>/dev/null") == 0);
#endif
    _hasGUISet = true;
    return _hasGUI;
}

} // namespace Ui
