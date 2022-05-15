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

#define getche _getwche

#else

#include <termios.h>
#include <unistd.h>

static int getche() {
    static struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON);
    newt.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int c = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
    return c;
}

#endif

int main(int argc, char** argv)
{
    const char* appName = argc>0 && argv[0] ? argv[0] : PopTracker::APPNAME;
    bool openConsole = false;
    bool printVersion = false;
    bool showHelp = false;
    bool badArg = false;
    bool cli = false;
    bool yes = false;
    bool no = false;
    bool listPacks = false;
    const char* installPack = nullptr;

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
        } else if (strcasecmp("--yes", argv[1])==0) {
            yes = true;
            no = false;
            argv++;
            argc--;
        } else if (strcasecmp("--no", argv[1])==0) {
            yes = false;
            no = true;
            argv++;
            argc--;
        } else if (strcasecmp("--list-packs", argv[1])==0) {
            listPacks = true;
            cli = true;
            argv++;
            argc--;
        } else if (argc>2 && strcasecmp("--install-pack", argv[1])==0) {
            installPack = argv[2];
            cli = true;
            argv+=2;
            argc-=2;
        } else if (strcasecmp("--install-pack", argv[1])==0) {
            badArg = true;
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

    if (showHelp || badArg) {
        printf("Usage: %s [--help] [--console] [--version] [--yes|--no] [--list-packs] [--install-pack <uid>]\n"
               "\n"
                "    --help: show this message\n"
                "    --console: try to open console window if not attached to a console (windows)\n"
                "    --version: print version and exit\n"
                "    --yes: answer yes to questions for cli tools\n"
                "    --no: answer no to questions for cli tools\n"
                "    --list-packs: list installed and installable packs\n"
                "    --install-pack <uid>: download and install with given uid from repositories\n"
                "\n", appName);
        return badArg ? 1 : 0;
    }

    if (printVersion) {
        printf("%s\n", PopTracker::VERSION_STRING);
        return 0;
    }

    PopTracker popTracker(argc, argv, cli);

    // CLI tools
    if (cli) {
        PackManager::confirmation_callback confirm;
        if (yes) {
            confirm = [](std::string, std::function<void(bool)> f) { f(true); };
        } else if (no) {
            confirm = [](std::string, std::function<void(bool)> f) { f(false); };
        } else {
            confirm = [](std::string msg, std::function<void(bool)> f) {
                int c = 0;
                do {
                    printf("%s [y/N] ", msg.c_str());
                    fflush(stdout);
                    c = ::toupper(getche());
                    if (c != '\n') printf("\n");
                } while (c != 'Y' && c != 'N' && c != '\n' && c != '\r' && c != EOF);
                f(c == 'Y');
            };
        }
        if (listPacks) {
            if (popTracker.ListPacks(confirm))
                return 0;
        }
        if (installPack) {
            if (popTracker.InstallPack(installPack, confirm))
                return 0;
        }
        return 1;
    }

    // run GUI
    printf("%s %s\n", PopTracker::APPNAME, PopTracker::VERSION_STRING);
    return popTracker.run();
}

