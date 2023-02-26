#ifndef _UILIB_DLG_H
#define _UILIB_DLG_H


#include <string>
#include <list>
#include <mutex>


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
    static bool OpenFile(const std::string& title, const std::string& dflt, const std::list<FileType>& types, std::string& out, bool multi=false);
    static bool SaveFile(const std::string& title, const std::string& dflt, const std::list<FileType>& types, std::string& out);

private:
    static std::mutex mutex; // not all of tinyfd is thread safe
};
    
} // namespace

#endif // _UILIB_DLG_H

