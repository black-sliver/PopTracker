#pragma once

#include <atomic>
#include <cstdio>
#include <mutex>
#include "../../src/core/fs.h"


class TempDir {
public:
    TempDir() = default;

    ~TempDir()
    {
        if (valid()) {
            fs::error_code ec;
            fs::remove_all(_base, ec);
            if (ec)
                fprintf(stderr, "WARNING: failed to cleanup TempDir: %s\n", ec.message().c_str());
        }
    }

    bool valid() const
    {
        return !_base.empty() && fs::is_directory(_base) && _base != _base.root_directory();
    }

    fs::path tempPath()
    {
        ensureInit();
        const unsigned n = _n.fetch_add(1);
        return fs::path(_base / std::to_string(n));
    }

    void ensureInit()
    {
        if (valid())
            return;

        std::lock_guard _(_initMutex);

        if (valid())
            return;

        if (!_base.empty())
            throw new std::runtime_error("TempDir already initialized but invalid");

        const auto tmplPath = fs::temp_directory_path() / "poptracker-test-XXXXXX";
#ifdef _WIN32
        // there appears to be no mkdtemp on Windows, so we emulate it and ignore the permissions
        auto tmpl = tmplPath.wstring();
        wchar_t* temp = _wmktemp_s(tmpl.data(), tmpl.length() + 1);
        if (!temp)
            throw new std::runtime_error("Failed to create temporary name");
        fs::path tempDir = temp;
        if (!fs::create_directories(tempDir))
            throw new std::runtime_error("Failed to create temporary directory: already exists");
        _base = tempDir;
#else
        auto tmpl = tmplPath.string();
        char* temp = mkdtemp(tmpl.data());
        if (!temp)
            throw new std::runtime_error("Failed to create temporary directory");
        _base = fs::u8path(temp);
#endif

        if (!valid())
            throw new std::runtime_error("Invalid temporary directory");
    }

private:
    std::mutex _initMutex;
    fs::path _base{};
    std::atomic<unsigned> _n = 0;
};
