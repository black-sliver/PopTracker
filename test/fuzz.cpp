#include "fuzz.hpp"
#include <cstdint>


extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, const size_t size) {
    const std::string_view sv = {reinterpret_cast<const char *>(data), size};
    for (const auto target: fuzzTargets) {
        target(sv);
    }
    return 0;  // Values other than 0 and -1 are reserved for future use.
}
