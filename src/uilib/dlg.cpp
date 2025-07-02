#include "dlg.h"

#if defined __WIN32__
#include <cassert>
#include <cstdint>
#include <windows.h>

#include <commdlg.h>
#else
#include <cstdio>
#include <tinyfiledialogs.h>

#include <tinyfiledialogs.cpp>
#endif


// NOTE: tinyfd_inputBox does not work in wine


namespace Ui {

std::mutex Dlg::_mutex;
bool Dlg::_hasGUI = false;
bool Dlg::_hasGUISet = false;


#ifdef __WIN32__
#define  ID_INPUT     200
#define  ID_INFOTEXT  201
#define  BUTTON_CLASS       0x0080
#define  EDIT_CLASS         0x0081
#define  STATIC_CLASS       0x0082

static intptr_t CALLBACK InputBoxDlgProc(HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    static char* buf = nullptr;
    static size_t buflen = 0;
    static HFONT hFont = nullptr;

    switch(uiMsg)
    {
        case WM_INITDIALOG:
        {
            void** data = (void**)lParam;
            buf = (char*)(data[0]);
            buflen = (size_t)data[1];
            HWND hEdit = GetDlgItem(hwnd, ID_INPUT);
            SendMessageW(hEdit, EM_SETSEL, 0, -1);
            SetFocus(hEdit);
            break;
        }

        case WM_SETFONT:
            hFont = (HFONT)wParam;
            break;

        case WM_CLOSE:
            DeleteObject(hFont);
            EndDialog(hwnd, IDCANCEL);
            hFont = nullptr;
            buf = nullptr;
            buflen = 0;
            break;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                {
                    if (buf && buflen) {
                        wchar_t tmp[256]; *tmp = 0;
                        HWND hEdit = GetDlgItem(hwnd, ID_INPUT);
                        GetWindowTextW(hEdit, tmp, sizeof(tmp)/2);
                        *buf = 0;
                        WideCharToMultiByte(CP_UTF8, 0, tmp, -1, buf, buflen, nullptr, nullptr);
                        buf = nullptr;
                        buflen = 0;
                    }
                    EndDialog(hwnd, IDOK);
                    DeleteObject(hFont);
                    hFont = nullptr;
                    break;
                }

                case IDCANCEL:
                {
                    EndDialog(hwnd, IDCANCEL);
                    DeleteObject(hFont);
                    hFont = nullptr;
                    buf = nullptr;
                    buflen = 0;
                    break;
                }
            }
        }
    }
    return FALSE;
}

static LPWORD DWORD_ALIGN(LPWORD lpIn)
{
   ULONG_PTR ul = (ULONG_PTR)lpIn;

   ul += 3;
   ul >>= 2;
   ul <<= 2;

   return (LPWORD)ul;
}

static LPVOID InitDialogU(LPVOID buf, ULONG_PTR buflen, LPCSTR title, DWORD style, WORD ctrlno, short x, short y, short cx, short cy)
{
    int nchar;

    if (sizeof(DLGTEMPLATE) + 4 > buflen) return nullptr;
    LPDLGTEMPLATE dlg = (LPDLGTEMPLATE)buf;
    dlg->style = style;
    dlg->dwExtendedStyle = 0;
    dlg->cdit = ctrlno;
    dlg->x  = x;  dlg->y  = y;
    dlg->cx = cx; dlg->cy = cy;

    // data starts after dialog
    LPWORD wbuf = (LPWORD)(dlg + 1);
    buflen -= (uint8_t*)wbuf - (uint8_t*)buf;

    // menu: None
    *wbuf = 0;
    wbuf++;

    // class
    *wbuf = 0;
    wbuf++;

    buflen -= 4;

    // title
    LPWSTR lpwTitle = (LPWSTR)wbuf;
    nchar = MultiByteToWideChar(CP_UTF8, 0, title, -1, lpwTitle, 0);
    if ((ULONG_PTR)nchar*2 > buflen) return nullptr;
    nchar = MultiByteToWideChar(CP_UTF8, 0, title, -1, lpwTitle, nchar);
    wbuf += nchar;
    buflen -= nchar*2;

    // font
    if(style & DS_SETFONT)
    {
       if (buflen < 2) return nullptr;
       *wbuf = 8; // 8pt
       wbuf++;
       buflen -= 2;
       wchar_t fontname[] = L"MS Shell Dlg";
       nchar = sizeof(fontname)/2;
       if ((ULONG_PTR)nchar*2 > buflen) return nullptr;
       memcpy(wbuf, fontname, nchar*2);
       wbuf += nchar;
       // buflen -= nchar*2;
    }

    return wbuf;
}

static LPVOID CreateDlgControlU(LPVOID buf, ULONG_PTR buflen, WORD ctrlclass, WORD id, LPCSTR text, DWORD style, short x, short y, short cx, short cy)
{
    int nchar;

    if (sizeof(DLGITEMTEMPLATE) + 6 > buflen) return nullptr;
    LPWORD wbuf = DWORD_ALIGN((LPWORD)buf); // 4-byte aligned
    LPDLGITEMTEMPLATE item = (LPDLGITEMTEMPLATE)wbuf;
    item->style = style;
    item->dwExtendedStyle = 0;
    item->x  = x ; item->y  = y;
    item->cx = cx; item->cy = cy;
    item->id = id;

    // data starts after dialog item
    wbuf = (LPWORD)(item + 1);
    buflen -= (uint8_t*)wbuf - (uint8_t*)buf;

    // class
    *wbuf = 0xFFFF;
    wbuf++;
    *wbuf = ctrlclass;
    wbuf++;
    buflen -= 4;

    // text
    LPWSTR lpwText = (LPWSTR)wbuf;
    nchar = MultiByteToWideChar(CP_UTF8, 0, text, -1, lpwText, 0);
    if ((ULONG_PTR)nchar*2 > buflen) return nullptr;
    nchar = MultiByteToWideChar(CP_UTF8, 0, text, -1, lpwText, nchar);
    wbuf += nchar;
    buflen -= nchar*2;

    // no creation data
    if (buflen < 4) return nullptr;
    wbuf = DWORD_ALIGN(wbuf-1)+1;
    *wbuf = 0;
    wbuf++;

    return wbuf;
}

static int InputBoxU(HWND hwnd, LPCSTR prompt, LPCSTR title, LPSTR textbuf, size_t textbuflen, bool password)
{
    // This builds a dialog template in memory
    // Dialog template format:
    // 1 DLGTEMPLATE followed by
    // some variable length "data" sections
    // n DLGITEMTEMPLATE each followed by
    // some variable length "data" sections
    // DLGITEMTEMPLATE have to be DWORD aligned (4 bytes)

    // TODO: after some thought this should probably move all strings to
    // Set*Text so we can easily define the template with structs on the stack

    DWORD style;
    LRESULT ret = 0;
    void* userdata[] = {textbuf, (void*)textbuflen};

    // Allocate buffer for dialog template
    ULONG_PTR dlgbuflen = 1024;
    LPVOID dlgbuf = malloc(dlgbuflen);
    memset(dlgbuf, 0, dlgbuflen);
    LPVOID bufp = dlgbuf;
    LPVOID endp = (uint8_t*)bufp + dlgbuflen;

    // Prepare the dialog box
    style = DS_SHELLFONT | DS_CENTER | DS_3DLOOK | WS_POPUP | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION | WS_VISIBLE;
    bufp = InitDialogU(bufp, dlgbuflen, title, style, 4, 0, 0, 190, 47);
    if (!bufp) goto err;
    dlgbuflen = (uint8_t*)endp - (uint8_t*)bufp;

    // OK-Button
    style = WS_CHILD | WS_VISIBLE | WS_TABSTOP |BS_DEFPUSHBUTTON;
    bufp = CreateDlgControlU(bufp, dlgbuflen, BUTTON_CLASS, IDOK, "OK", style, 135, 7, 48, 15);
    if (!bufp) goto err;
    dlgbuflen = (uint8_t*)endp - (uint8_t*)bufp;

    // Cancel-Button
    style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON;
    bufp = CreateDlgControlU(bufp, dlgbuflen, BUTTON_CLASS, IDCANCEL, "Cancel", style, 135, 26, 48, 15);
    if (!bufp) goto err;
    dlgbuflen = (uint8_t*)endp - (uint8_t*)bufp;

    // Label
    style = WS_CHILD | WS_VISIBLE | SS_LEFT;
    bufp = CreateDlgControlU(bufp, dlgbuflen, STATIC_CLASS, ID_INFOTEXT, prompt, style, 10, 9, 120, 16);
    if (!bufp) goto err;
    dlgbuflen = (uint8_t*)endp - (uint8_t*)bufp;

    // Textbox
    style = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL;
    if (password) style |= ES_PASSWORD;
    bufp = CreateDlgControlU(bufp, dlgbuflen, EDIT_CLASS, ID_INPUT, textbuf, style, 10, 26, 120, 13);
    if (!bufp) goto err;

    ret = DialogBoxIndirectParamW(NULL, (LPDLGTEMPLATE)dlgbuf, hwnd,
                                  (DLGPROC)InputBoxDlgProc, (LPARAM)userdata);

err:
    free(dlgbuf);
    return (ret > 0) ? ret : 0;
}
#endif

bool Dlg::InputBox(const std::string& title, const std::string& message, const std::string& dflt, std::string& result, bool password)
{
#ifdef __WIN32__
    char buf[256]; memset(buf, 0, sizeof(buf));
    memcpy(buf, dflt.c_str(), std::min(dflt.length()+1, sizeof(buf)-1));
    int res = InputBoxU(nullptr, message.c_str(), title.c_str(), buf, 256, password);
    buf[255] = 0;
    if (res != IDOK) return false;
    result = buf;
    return true;
#else
    std::lock_guard lock(_mutex);
    // NOTE: some of tinyfd's UI providers do not like `'` and/or `\` in the default. The output is fine though.
    // TODO: switch to a different dialog system for Linux and macOS
    const bool defaultInvalid = dflt.find_first_of("'\\") != std::string::npos;
    const char* res = tinyfd_inputBox(title.c_str(), message.c_str(),
        password ? nullptr : defaultInvalid ? "" : dflt.c_str());
    if (res)
        result = res;
    return !!res;
#endif
}

Dlg::Result Dlg::MsgBox(const std::string& title, const std::string& message, Dlg::Buttons btns, Dlg::Icon icon, Dlg::Result dflt)
{
#ifdef __WIN32__
    UINT type = btns == Buttons::OKCancel ? MB_OKCANCEL :
                btns == Buttons::YesNo ? MB_YESNO :
                btns == Buttons::YesNoCancel ? MB_YESNOCANCEL : MB_OK;

    size_t maxlen;
    maxlen = title.length()*2 + 2;
    LPWSTR lpwTitle = (LPWSTR)malloc(maxlen);
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, lpwTitle, maxlen);
    maxlen = message.length()*2 + 2;
    LPWSTR lpwMessage = (LPWSTR)malloc(maxlen);
    MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, lpwMessage, maxlen);

    if (icon == Icon::Info) type |= MB_ICONINFORMATION;
    if (icon == Icon::Warning) type |= MB_ICONWARNING;
    if (icon == Icon::Error) type |= MB_ICONERROR;
    if (icon == Icon::Question) type |= MB_ICONQUESTION;
    if (btns == Buttons::YesNo && dflt != Result::Yes) type |= MB_DEFBUTTON2;
    if (btns == Buttons::OKCancel && dflt != Result::OK) type |= MB_DEFBUTTON2;
    if (btns == Buttons::YesNoCancel && dflt == Result::No) type |= MB_DEFBUTTON2;
    if (btns == Buttons::YesNoCancel && dflt == Result::Cancel) type |= MB_DEFBUTTON3;

    int res = MessageBoxW(nullptr, lpwMessage, lpwTitle, type);

    free(lpwTitle);
    free(lpwMessage);

    if (res == IDOK) return Dlg::Result::OK;
    if (res == IDCANCEL) return Dlg::Result::Cancel;
    if (res == IDYES) return Dlg::Result::Yes;
    if (res == IDNO) return Dlg::Result::No;

    return Dlg::Result::Cancel;
#else
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
#endif
}

bool Dlg::OpenFile(const std::string& title, const fs::path& dflt, const std::list<FileType>& types, fs::path& out, bool multi)
{
#ifdef __WIN32__
    using namespace std::string_literals;

    // TODO: implement multi-select
    assert(!multi);
    bool res = false;
    wchar_t buf[MAX_PATH];
    memset(buf, 0, sizeof(buf));

    if (dflt.native().length() >= MAX_PATH)
        return false;

    std::string filter; // "Name\0*.ext\0All Files\0*\0"s
    if (types.empty())
        filter = "All Files\0*\0"s;
    for (const auto& type: types) {
        std::string patterns;
        for(const auto& pat: type.patterns) {
            if (!patterns.empty()) patterns += ";";
            patterns += pat;
        }
        filter += type.name + "\0"s + patterns + "\0"s;
    }

    LPWSTR lpwTitle = (LPWSTR)malloc(title.length()*2+2);
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, lpwTitle, title.length()+1);
    LPWSTR lpwFilter = (LPWSTR)malloc(filter.length()*2+2);
    MultiByteToWideChar(CP_UTF8, 0, filter.c_str(), filter.length()+1, lpwFilter, filter.length()+1);

    static_assert(sizeof(*buf) == sizeof(*dflt.c_str()));
    memcpy(buf, dflt.c_str(), sizeof(*buf) * dflt.native().length()); // terminating NUL from memset
    assert(buf[dflt.native().length()] == 0);

    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = lpwFilter;
    ofn.lpstrTitle = lpwTitle;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameW(&ofn)) {
        out = ofn.lpstrFile;
        res = true;
    }
    free(lpwTitle);
    free(lpwFilter);
    return res;
#else
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
#endif
}

bool Dlg::SaveFile(const std::string& title, const fs::path& dflt, const std::list<FileType>& types, fs::path& out)
{
#ifdef __WIN32__
    using namespace std::string_literals;

    // NOTE: unicode filename currently not supported since we use fopen (not CreateFileW)
    bool res = false;
    wchar_t buf[MAX_PATH];
    memset(buf, 0, sizeof(buf));

    if (dflt.native().length() >= MAX_PATH)
        return false;

    std::string filter; // "Name\0*.ext\0All Files\0*\0"s
    if (types.empty()) filter = "All Files\0*\0"s;
    for (const auto& type: types) {
        std::string patterns;
        for(const auto& pat: type.patterns) {
            if (!patterns.empty()) patterns += ";";
            patterns += pat;
        }
        filter += type.name + "\0"s + patterns + "\0"s;
    }

    static_assert(sizeof(*buf) == sizeof(*dflt.c_str()));
    memcpy(buf, dflt.c_str(), sizeof(*buf) * dflt.native().length()); // terminating NUL from memset
    assert(buf[dflt.native().length()] == 0);

    LPWSTR lpwTitle = (LPWSTR)malloc(title.length()*2+2);
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, lpwTitle, title.length()+1);
    LPWSTR lpwFilter = (LPWSTR)malloc(filter.length()*2+2);
    MultiByteToWideChar(CP_UTF8, 0, filter.c_str(), filter.length()+1, lpwFilter, filter.length()+1);

    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = buf;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = lpwFilter;
    ofn.lpstrTitle = lpwTitle;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (GetSaveFileNameW(&ofn)) {
        out = ofn.lpstrFile;
        res = true;
    }
    free(lpwTitle);
    free(lpwFilter);
    return res;
#else
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
#endif
}

static int tryRun(const char* cmd)
{
    FILE *p = popen(cmd, "r");
    if (!p)
        return -1;
    return pclose(p);
}

/// Returns true if running a dialog won't read from stdin
bool Dlg::hasGUI()
{
    if (_hasGUISet)
        return _hasGUI;
#if defined __WIN32__ || defined __APPLE__
    // assume yes for windows and mac
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
