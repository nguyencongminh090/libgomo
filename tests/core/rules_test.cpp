#include <doctest.h>

#include "pygomo/core/rules.hpp"

using namespace pygomo;

TEST_CASE("Default rules are freestyle with a free opening and no move limit") {
    Rules r;
    CHECK(r.board == BoardRule::Freestyle);
    CHECK(r.opening == OpeningRule::FreeOpen);
    CHECK_FALSE(r.maxMoves.has_value());
    CHECK(validate(r).has_value());
}

TEST_CASE("Board rule and opening rule vary independently") {
    Rules a{BoardRule::Renju,    OpeningRule::Swap2};
    Rules b{BoardRule::Renju,    OpeningRule::FreeOpen};
    Rules c{BoardRule::Standard, OpeningRule::Swap2};
    CHECK(a != b);
    CHECK(a != c);
    CHECK(a == Rules{BoardRule::Renju, OpeningRule::Swap2});
}

TEST_CASE("maxMoves must be positive when present") {
    CHECK(validate(Rules{.maxMoves = 225}).has_value());
    for (int bad : {0, -1}) {
        auto r = validate(Rules{.maxMoves = bad});
        REQUIRE_FALSE(r.has_value());
        CHECK(r.error() == RulesError::InvalidMaxMoves);
    }
}

TEST_CASE("maxMoves takes part in equality") {
    CHECK(Rules{.maxMoves = 10} != Rules{});
    CHECK(Rules{.maxMoves = 10} == Rules{.maxMoves = 10});
}
