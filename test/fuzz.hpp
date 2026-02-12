#pragma once

#include <string_view>
#include <vector>

typedef void (*FuzzTarget)(std::string_view);

inline std::vector<FuzzTarget> fuzzTargets;

class Fuzz
{
public:
    explicit Fuzz(const FuzzTarget target)
    {
        fuzzTargets.push_back(target);
    }
};

#define FUZZ(f) [[maybe_unused]] static Fuzz _fuzz ## f { f };
