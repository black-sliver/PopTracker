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
#if defined WIN32 || defined _WIN32
    // use --console to force open a dos window
    bool openConsole = false;
    if (argc>1 && strcasecmp("--console", argv[1])==0) {
        openConsole = true;
        argv++;
        argc--;
    }
    // enable stdout on windows
    if(AttachConsole(ATTACH_PARENT_PROCESS) || (openConsole && AllocConsole())) {
        freopen("CONOUT$", "w", stdout);
        setvbuf(stdout, NULL, _IONBF, 0); // _IOLBF
        freopen("CONOUT$", "w", stderr);
        setvbuf(stderr, NULL, _IONBF, 0);
    }
    EnableVisualStyles();
#endif

    if (argc>1 && strcasecmp("--version", argv[1])==0) {
        printf("%s\n", PopTracker::VERSION_STRING);
        return 0;
    }
    printf("%s %s\n", PopTracker::APPNAME, PopTracker::VERSION_STRING);
    PopTracker popTracker(argc, argv);
    return popTracker.run();
}

