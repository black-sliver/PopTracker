// different translation unit than test_singleton
#include <string>
#include "../../src/core/singleton.hpp"

void* singleton2 = &pop::Singleton<std::string>::get();
