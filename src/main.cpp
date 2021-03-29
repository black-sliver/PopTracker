#include "poptracker.h"
#include <string.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc>1 && strcasecmp("--version", argv[1])==0) {
        printf("%s\n", PopTracker::VERSION_STRING);
        return 0;
    }
    PopTracker popTracker(argc, argv);
    return popTracker.run();
}

