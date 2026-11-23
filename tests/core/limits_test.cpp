#include <doctest.h>

#include "gomo/core/limits.hpp"

using namespace gomo;
using namespace std::chrono_literals;

TEST_CASE("Default limits impose nothing") {
    Limits l;
    CHECK_FALSE(l.turnTime.has_value());
    CHECK_FALSE(l.matchTime.has_value());
    CHECK_FALSE(l.maxDepth.has_value());
    CHECK_FALSE(l.maxNodes.has_value());
}

TEST_CASE("Limits compare field by field") {
    Limits a{.turnTime = 5000ms, .maxDepth = 12};
    CHECK(a == Limits{.turnTime = 5000ms, .maxDepth = 12});
    CHECK(a != Limits{.turnTime = 5000ms});
    CHECK(a != Limits{.turnTime = 5001ms, .maxDepth = 12});
}

TEST_CASE("maxNodes holds values beyond 32 bits") {
    Limits l{.maxNodes = 10'000'000'000LL};
    REQUIRE(l.maxNodes.has_value());
    CHECK(*l.maxNodes == 10'000'000'000LL);
}
