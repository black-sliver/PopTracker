// different translation unit than singleton2
#include <string>
#include <gtest/gtest.h>
#include "../../src/core/singleton.hpp"

extern void* singleton2;

TEST(Singleton, IsSingleton) {
    EXPECT_EQ(&pop::Singleton<std::string>::get(), &pop::Singleton<std::string>::get());
}

TEST(Singleton, AcrossTranslationUnits) {
    EXPECT_EQ(&pop::Singleton<std::string>::get(), singleton2);
}
