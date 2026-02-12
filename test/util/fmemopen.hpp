#pragma once

#if defined _WIN32 || defined WIN32
// Windows does not have fmemopen
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <share.h> // _SH_DENYRW
#include <cstdio>

static FILE* fmemopen(const void* buf, const size_t len, const char* modes)
{
    char tempDir[MAX_PATH - 13];
    char tempFile[MAX_PATH + 1];
    if (!GetTempPathA(sizeof(tempDir), tempDir))
        return NULL;
    if (!GetTempFileNameA(tempDir, "Mem", 0, tempFile))
        return NULL;
    int fd;
    auto err = _sopen_s(&fd, tempFile, _O_CREAT | _O_SHORT_LIVED | _O_TEMPORARY | _O_RDWR | _O_BINARY | _O_NOINHERIT,
        _SH_DENYRW, _S_IREAD | _S_IWRITE);
    if (err != 0)
        return NULL;
    if (fd == -1)
        return NULL;
    FILE* f = _fdopen(fd, "wb+");
    (void)modes;
    if (!f) {
        _close(fd);
        return NULL;
    }
    if (fwrite(buf, len, 1, f) != 1u) {
        fclose(f);
        return NULL;
    }
    rewind(f);
    return f;
}
#endif
