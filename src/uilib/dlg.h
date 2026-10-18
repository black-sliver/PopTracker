#pragma once

#include <list>
#include <string>

#include "../core/fs.h"


namespace Ui {

class Dlg {
public:
    Dlg() = delete;

    enum class Result {
        No = 0,
        Yes = 1,
        OK = 2,
        Cancel = -1
    };

    enum class Buttons {
        OK, OKCancel, YesNo, YesNoCancel
    };

    enum class Icon {
        None = 0,
        Info,
        Warning,
        Error,
        Question
    };

    struct FileType {
        std::string name;
        std::list<std::string> patterns;
    };

    static bool InputBox(const std::string& title, const std::string& message, const std::string& dflt, std::string& result, bool password=false);
    static Result MsgBox(const std::string& title, const std::string& message, Buttons btns=Buttons::OK, Icon icon=Icon::Info, Result dflt=Result::OK);
    static bool OpenFile(const std::string& title, const fs::path& dflt, const std::list<FileType>& types, fs::path& out, bool multi=false);
    static bool SaveFile(const std::string& title, const fs::path& dflt, const std::list<FileType>& types, fs::path& out);

    /// Returns true if running a dialog won't read from stdin
    static bool hasGUI();
};

} // namespace
