#include "poptracker.h"
#include <string.h>
#include <stdio.h>


#if defined WIN32 || defined _WIN32
#include <windows.h>

// as per https://stackoverflow.com/questions/4308503
ULONG_PTR EnableVisualStyles(VOID)
{
    TCHAR dir[MAX_PATH];
    ULONG_PTR ulpActivationCookie = FALSE;
    ACTCTX actCtx =
    {
        sizeof(actCtx),
        ACTCTX_FLAG_RESOURCE_NAME_VALID
            | ACTCTX_FLAG_SET_PROCESS_DEFAULT
            | ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID,
        TEXT("shell32.dll"), 0, 0, dir, (LPCTSTR)124
    };
    UINT cch = GetSystemDirectory(dir, sizeof(dir) / sizeof(*dir));
    if (cch >= sizeof(dir) / sizeof(*dir)) { return FALSE; /*shouldn't happen*/ }
    dir[cch] = TEXT('\0');
    ActivateActCtx(CreateActCtx(&actCtx), &ulpActivationCookie);
    return ulpActivationCookie;
}
#endif

int main(int argc, char** argv)
{
    const char* appName = argc>0 && argv[0] ? argv[0] : PopTracker::APPNAME;
    bool openConsole = false;
    bool printVersion = false;
    bool showHelp = false;

    while (argc > 1) {
        if (strcasecmp("--console", argv[1])==0) {
            // use --console to force open a dos window
            openConsole = true;
            argv++;
            argc--;
        } else if (strcasecmp("--version", argv[1])==0) {
            printVersion = true;
            argv++;
            argc--;
        } else if (strcasecmp("--help", argv[1])==0) {
            showHelp = true;
            argv++;
            argc--;
        }
    }

#if defined WIN32 || defined _WIN32
    // enable stdout on windows
    if(AttachConsole(ATTACH_PARENT_PROCESS) || (openConsole && AllocConsole())) {
        freopen("CONOUT$", "w", stdout);
        setvbuf(stdout, NULL, _IONBF, 0); // _IOLBF
        freopen("CONOUT$", "w", stderr);
        setvbuf(stderr, NULL, _IONBF, 0);
    }
    EnableVisualStyles();
#else
    (void)openConsole; // no-op on non-windows
#endif

    if (showHelp) {
        printf("Usage: %s [--help] [--console] [--version]\n"
               "\n"
                "    --help: show this message\n"
                "    --console: try to open console window if not attached to a console (windows)\n"
                "    --version: print version and exit\n"
                "\n", appName);
        return 0;
    }

    if (printVersion) {
        printf("%s\n", PopTracker::VERSION_STRING);
        return 0;
    }

    PopTracker popTracker(argc, argv);

    // run GUI
    printf("%s %s\n", PopTracker::APPNAME, PopTracker::VERSION_STRING);
    return popTracker.run();
}

