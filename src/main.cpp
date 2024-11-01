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
    bool listInstalled = false;
    bool listInstallable = false;
    const char* installPack = nullptr;
    const char* loadPack = nullptr;
    const char* packPath = nullptr;
    const char* packVersion = nullptr;
    const char* packVariant = nullptr;

    while (argc > 1) {
        if (strcasecmp("--console", argv[1])==0) {
            // use --console to force open a dos window
            openConsole = true;
        } else if (strcasecmp("--version", argv[1])==0) {
            printVersion = true;
        } else if (strcasecmp("--help", argv[1])==0) {
            showHelp = true;
        } else if (strcasecmp("--yes", argv[1])==0) {
            yes = true;
            no = false;
        } else if (strcasecmp("--no", argv[1])==0) {
            yes = false;
            no = true;
        } else if (strcasecmp("--list-packs", argv[1])==0) {
            listInstallable = true;
            listInstalled = true;
            cli = true;
        } else if (strcasecmp("--list-installed", argv[1])==0) {
            listInstalled = true;
            cli = true;
        } else if (strcasecmp("--install-pack", argv[1])==0) {
            if (argc <= 2) {
                badArg = true;
                break;
            }
            installPack = argv[2];
            cli = true;
            argv++;
            argc--;
        } else if (strcasecmp("--load-pack", argv[1]) == 0) {
            if (argc <= 2) {
                badArg = true;
                break;
            }
            loadPack = argv[2];
            argv++;
            argc--;
        } else if (strcasecmp("--pack-version", argv[1]) == 0) {
            if (argc <= 2) {
                badArg = true;
                break;
            }
            packVersion = argv[2];
            argv++;
            argc--;
        } else if (strcasecmp("--pack-variant", argv[1]) == 0) {
            if (argc <= 2) {
                badArg = true;
                break;
            }
            packVariant = argv[2];
            argv++;
            argc--;
        } else if (argc==2) {
            packPath = argv[1];
        } else {
            badArg = true;
            break;
        }
        argv++;
        argc--;
    }

    if ((packPath && cli) || (packPath && loadPack) || (cli && loadPack)) badArg = true;

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
        printf("Usage: %s [<ARGS...>] [<ACTION ARGS...>] [<ACTION>|<path/to/pack>]\n"
               "\n"
               "  path/to/pack: will try to load this pack on startup\n"
               "        Action args: --pack-variant\n"
               "\n"
               "  Args:\n"
               "    --console: try to open console window if not attached to a console (windows)\n"
               "    --yes: answer yes to questions for cli tools\n"
               "    --no: answer no to questions for cli tools\n"
               "\n"
               "  Actions:\n"
               "    --version: print version and exit\n"
               "    --help: show this message\n"
               "    --list-packs: list installed and installable packs\n"
               "    --list-installed: list only installed packs\n"
               "    --install-pack <uid>: download and install pack with uid from repositories\n"
               "    --load-pack <uid>[:<version>]: load this pack on startup\n"
               "        Action args: --pack-variant, --pack-version\n"
               "\n"
               "  Action args:\n"
               "    --pack-variant <variant>: try to load this variant\n"
               "    --pack-version <version>: try to load this version\n"
               "\n", appName);
        return badArg ? 1 : 0;
    }

    if (printVersion) {
        printf("%s\n", PopTracker::VERSION_STRING);
        return 0;
    }

    // argc/argv should be empty here. we pass everything through json args
    json args = json::object();
    if (packPath) {
        args["pack"] = { {"path", localToUTF8(packPath)} };
    } else if (loadPack) {
        args["pack"] = { {"uid", loadPack} };
    }
    if ((packPath || loadPack) && packVariant) {
        args["pack"]["variant"] = packVariant;
    }
    if (loadPack && packVersion) {
        args["pack"]["version"] = packVersion;
    }

    PopTracker popTracker(argc, argv, cli, args);

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
        if (listInstalled || listInstallable) {
            if (popTracker.ListPacks(confirm, listInstallable))
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

