#include "../../../src/gh/api/releases.hpp"
#include <gtest/gtest.h>
#include "releases_data.h"


TEST(GHAPIReleases, Iter) {
    int count = 0;
    for (const auto& _: gh::api::Releases{releasesData}) {
        count++;
    }
    ASSERT_EQ(count, 2);
}

TEST(GHAPIReleases, ParseRelease) {
    for (const auto& rls: gh::api::Releases{releasesData}) {
        ASSERT_EQ(rls.tagName()[0], 'v');
    }
}
