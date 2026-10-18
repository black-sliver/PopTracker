#import "dlg.h"

#if __has_include(<UniformTypeIdentifiers/UniformTypeIdentifiers.h>)
#	define HAS_UTTYPE
#endif

extern "C" {
    #import <AppKit/AppKit.h>
    #ifdef HAS_UTTYPE
	#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
	#endif
}

#include <cassert>
#include <list>
#include <map>
#include <string>

#include "../core/fs.h"

#ifndef __APPLE__
#warning "GNUStep support is for testing only."
#endif


namespace Ui {

#ifdef HAS_UTTYPE
static std::map<std::string, std::string> contentTypeMap = {
    {"json", "public.json"},
    {"zip", "public.zip"},
};
#endif

void setTitle(NSSavePanel *dialog, const std::string& title)
{
    if (!title.empty())
        dialog.title = [NSString stringWithUTF8String: title.c_str()];
}

void setDflt(NSSavePanel *dialog, const fs::path& dflt)
{
    if (!dflt.empty()) {
        auto dfltDirPath = dflt.parent_path();
        if (!dfltDirPath.empty()) {
            NSString *dfltDir = [NSString stringWithUTF8String: dfltDirPath.u8string().c_str()];
            NSURL *dfltDirURL = [NSURL fileURLWithPath: dfltDir];
            dialog.directoryURL = dfltDirURL;
        }
        NSString *dfltName = [NSString stringWithUTF8String: dflt.filename().u8string().c_str()];
        dialog.nameFieldStringValue = dfltName;
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// NOTE: setAllowedFileTypes is deprecated, but we fall back to it and GNUStep doesn't have UTType.

static void setTypes(NSSavePanel *dialog, const std::list<Dlg::FileType>& types)
{
    if (types.empty())
        return;

    bool hasAny = false;
    NSMutableArray *fileTypes = [NSMutableArray new];
#ifdef HAS_UTTYPE
    NSMutableArray *contentTypes = [NSMutableArray new];
#endif
    for (const auto& t: types) {
        for (const auto& pat: t.patterns) {
            if (pat == "*" || pat == "*.*") {
                hasAny = true;
            } else {
                const char* p;
                if (pat[0] == '*' && pat[1] == '.')
                    p = pat.c_str() + 2;
                else if (pat[0] == '.')
                    p = pat.c_str() + 1;
                else
                    p = pat.c_str();
                NSString *aType = [NSString stringWithUTF8String: p];
                [fileTypes addObject: aType];
#ifdef HAS_UTTYPE
                if (@available(macOS 11.0, *)) {
                    auto ctIt = contentTypeMap.find(p);
                    if (ctIt != contentTypeMap.end()) {
                        NSString *tid = [NSString stringWithUTF8String: ctIt->second.c_str()];
                        [contentTypes addObject: [UTType typeWithIdentifier: tid]];
                    }
                }
#endif
            }
        }
    }

#ifdef HAS_UTTYPE
    bool useFileTypes = true;
    if (@available(macOS 11.0, *)) {
        if ([contentTypes count] && [contentTypes count] >= [fileTypes count]
            && [dialog respondsToSelector: @selector(setAllowedContentTypes:)]
        ) {
            [dialog setAllowedContentTypes: [NSArray arrayWithArray: contentTypes]];
            useFileTypes = false;
        }
    }

    if (useFileTypes)
#endif
    {
        if ([fileTypes count]) {
            if ([dialog respondsToSelector: @selector(setAllowedFileTypes:)])
                [dialog setAllowedFileTypes: [NSArray arrayWithArray: fileTypes]];
            else
                hasAny = true; // TODO: warn
        }
    }

#ifdef HAS_UTTYPE
    [contentTypes release];
#endif
    [fileTypes release];

    [dialog setAllowsOtherFileTypes: hasAny];
}

#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
// NOTE: GNUStep doesn't have the new names yet. Fix this whenever this can't be compiled on macOS anymore.

static NSAlertStyle styleFromIcon(Dlg::Icon icon)
{
    // NOTE: the intention of alertStyle does not really match the intention of icon:
    //       for example, question could map to different styles depending on what the question is.

    switch (icon) {
        case Dlg::Icon::None:
        case Dlg::Icon::Info:
        case Dlg::Icon::Question:
            return NSInformationalAlertStyle;
        case Dlg::Icon::Warning:
            return NSWarningAlertStyle;
        case Dlg::Icon::Error:
            return NSCriticalAlertStyle;
    }

    return NSInformationalAlertStyle; // unreachable
}

#pragma GCC diagnostic pop


bool Dlg::InputBox(const std::string& title, const std::string& message, const std::string& dflt, std::string& result,
                   const bool password)
{
    [NSApp activateIgnoringOtherApps: YES];
    NSAlert *alert = [NSAlert new];
    alert.alertStyle = styleFromIcon(Icon::None);
    alert.messageText = [NSString stringWithUTF8String: title.c_str()];
    alert.informativeText = [NSString stringWithUTF8String: message.c_str()];
    [alert addButtonWithTitle: [NSString stringWithUTF8String: "OK"]];
    [alert addButtonWithTitle: [NSString stringWithUTF8String: "Cancel"]];

    NSTextField* textField;
#ifdef __APPLE__
    if (password) {
        textField = [NSSecureTextField textFieldWithString: [NSString stringWithUTF8String: dflt.c_str()]];
    } else {
        textField = [NSTextField textFieldWithString: [NSString stringWithUTF8String: dflt.c_str()]];
    }
    // resize textField so it's fixed width = 12 x default height
    auto h = textField.frame.size.height;
    [textField setFrameSize: NSMakeSize(12 * h, h)];
#else
    // this may leak memory, but this codepath is for testing only anyway
    textField = [[NSTextField alloc] initWithFrame: NSMakeRect(0, 0, 200, 15)];
    [textField setStringValue: [NSString stringWithUTF8String: dflt.c_str()]];
    [textField setEditable: YES];
    [textField setSelectable: YES];
    (void)password; // not implemented since this should be dead anyway
#endif
    // TODO: add support for placeHolder in Dlg::InputBox

    // FIXME: GNUstep does not have accessoryView
#ifdef __APPLE__ // for testing we simply don't add the accessory view with GNUStep
    alert.accessoryView = textField;
#endif
    [textField selectText: nullptr];

    if ([alert respondsToSelector: @selector(layout)])
        [alert layout];
    [alert.window makeFirstResponder: textField];

    auto res = [alert runModal];
    if (res == NSAlertFirstButtonReturn) {
        auto text = [textField stringValue];
        result = [text UTF8String];
    }
    [alert release];
    return (res == NSAlertFirstButtonReturn);
}

Dlg::Result Dlg::MsgBox(const std::string& title, const std::string& message, const Dlg::Buttons btns,
                        const Dlg::Icon icon, const Dlg::Result dflt)
{
    [NSApp activateIgnoringOtherApps: YES];
    NSAlert *alert = [NSAlert new];
    alert.alertStyle = styleFromIcon(icon);
    alert.messageText = [NSString stringWithUTF8String: title.c_str()];
    alert.informativeText = [NSString stringWithUTF8String: message.c_str()];
    std::map<NSModalResponse, Result> resultMap;
    switch (btns) {
        case Buttons::OK:
            resultMap[NSModalResponseOK] = Result::OK; // dismissal is OK if there is only OK
            resultMap[NSAlertFirstButtonReturn] = Result::OK;
            [alert addButtonWithTitle: [NSString stringWithUTF8String: "OK"]];
            break;
        case Buttons::OKCancel:
            resultMap[NSAlertFirstButtonReturn] = Result::OK;
            resultMap[NSAlertSecondButtonReturn] = Result::Cancel;
            [alert addButtonWithTitle: [NSString stringWithUTF8String: "OK"]];
            [alert addButtonWithTitle: [NSString stringWithUTF8String: "Cancel"]];
            break;
        case Buttons::YesNo:
            resultMap[NSAlertFirstButtonReturn] = Result::Yes;
            resultMap[NSAlertSecondButtonReturn] = Result::No;
            [alert addButtonWithTitle: [NSString stringWithUTF8String: "Yes"]];
            [alert addButtonWithTitle: [NSString stringWithUTF8String: "No"]];
            break;
        case Buttons::YesNoCancel:
            resultMap[NSAlertFirstButtonReturn] = Result::Yes;
            resultMap[NSAlertSecondButtonReturn] = Result::No;
            resultMap[NSAlertThirdButtonReturn] = Result::Cancel;
            [alert addButtonWithTitle: [NSString stringWithUTF8String: "Yes"]];
            [alert addButtonWithTitle: [NSString stringWithUTF8String: "No"]];
            [alert addButtonWithTitle: [NSString stringWithUTF8String: "Cancel"]];
            break;
    }

    auto res = [alert runModal];
    [alert release];
    auto it = resultMap.find(res);
    if (it == resultMap.end())
        return dflt;
    return it->second;
}

bool Dlg::OpenFile(const std::string& title, const fs::path& dflt, const std::list<FileType>& types, fs::path& out,
                   bool multi)
{
    // TODO: implement multi-select
    assert(!multi);
    if (multi)
        return false; // Not implemented

    // Since this function is blocking, we can just use a new memory pool
    // and not care about the parent memory management.
    NSAutoreleasePool *pool = [NSAutoreleasePool new];

    // We assume that a NSApplication was already created, so NSApp global should be populated.
    [NSApp activateIgnoringOtherApps: YES];
    NSApplication *app = NSApp;
    NSWindow *parent = [app keyWindow];
    NSOpenPanel *dialog = [NSOpenPanel openPanel];

    dialog.canChooseDirectories = NO;

    setTitle(dialog, title);
    setDflt(dialog, dflt);
    setTypes(dialog, types);

    bool res = false;
    if ([dialog runModal] == NSModalResponseOK){
        out = [[[dialog URL] path] UTF8String];
        res = true;
    }

    [pool release];
    if (parent)
        [parent makeKeyAndOrderFront: nil];

    return res;
}

bool Dlg::SaveFile(const std::string& title, const fs::path& dflt, const std::list<FileType>& types, fs::path& out)
{
    // Since this function is blocking, we can just use a new memory pool
    // and not care about the parent memory management.
    NSAutoreleasePool *pool = [NSAutoreleasePool new];

    // We assume that a NSApplication was already created, so NSApp global should be populated.
    [NSApp activateIgnoringOtherApps: YES];
    NSApplication *app = NSApp;
    NSWindow *parent = [app keyWindow];
    NSSavePanel *dialog = [NSSavePanel savePanel];

    setTitle(dialog, title);
    setDflt(dialog, dflt);
    setTypes(dialog, types);

    bool res = false;
    if ([dialog runModal] == NSModalResponseOK){
        out = [[[dialog URL] path] UTF8String];
        res = true;
    }

    [pool release];
    if (parent)
        [parent makeKeyAndOrderFront: nil];

    return res;
}

bool Dlg::hasGUI()
{
    return true;
}

} // namespace Ui
