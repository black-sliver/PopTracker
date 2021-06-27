#include "poptracker.h"
#include <string.h>
#include <stdio.h>


#if defined WIN32 || defined _WIN32
#include <windows.h>
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
        freopen("CONOUT$", "w", stderr);
    }
#endif

    if (argc>1 && strcasecmp("--version", argv[1])==0) {
        printf("%s\n", PopTracker::VERSION_STRING);
        return 0;
    }
    printf("%s %s\n", PopTracker::APPNAME, PopTracker::VERSION_STRING);
    PopTracker popTracker(argc, argv);
    return popTracker.run();
}

