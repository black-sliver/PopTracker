#pragma once

#include <string_view>
#include <vector>

#ifndef __has_feature
    #define __has_feature(x) 0
#endif

#if (defined(__SANITIZE_ADDRESS__) || __has_feature(address_sanitizer)) \
        && __has_include(<sanitizer/lsan_interface.h>)
    #define WITH_LSAN
    #include <sanitizer/lsan_interface.h>
#endif


typedef void (*FuzzTarget)(std::string_view);

struct Fuzz
{
    static inline std::vector<FuzzTarget> targets;

    explicit Fuzz(const FuzzTarget target)
    {
        #ifdef WITH_LSAN
        // somehow the vector leaks memory with clang 22.1.8
        __lsan::ScopedDisabler lsanDisabler;
        #endif

        targets.push_back(target);
    }
};

#define FUZZ(f) [[maybe_unused]] static Fuzz _fuzz ## f { f };
