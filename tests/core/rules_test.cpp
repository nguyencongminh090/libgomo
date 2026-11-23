#include <doctest.h>

#include "gomo/core/rules.hpp"

using namespace gomo;

TEST_CASE("Default rules are freestyle with a free opening and no move limit") {
    Rules r;
    CHECK(r.board == BoardRule::Freestyle);
    CHECK(r.opening == OpeningRule::FreeOpen);
    CHECK_FALSE(r.maxMoves.has_value());
    CHECK(validate(r).has_value());
}

TEST_CASE("Board rule and opening rule vary independently") {
    Rules a{BoardRule::Renju, OpeningRule::Swap2};
    Rules b{BoardRule::Renju, OpeningRule::FreeOpen};
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

TEST_CASE("ext05_rules: equality compares extension keys and values") {
    const Rules rules{.extensions = {{"custom-rule", true}}};
    CHECK(rules == Rules{.extensions = {{"custom-rule", true}}});
    CHECK(rules != Rules{.extensions = {{"custom-rule", false}}});
    CHECK(rules != Rules{.extensions = {{"other-rule", true}}});
    CHECK(rules != Rules{});
}

TEST_CASE("ext05_rules: validation accepts extensions") {
    const Rules rules{.maxMoves = 225, .extensions = {{"custom-rule", true}}};
    CHECK(validate(rules).has_value());
}

TEST_CASE("ext05_rules: default and designated initialization leave extensions empty") {
    CHECK(Rules{}.extensions.empty());
    const Rules rules{.maxMoves = 225};
    CHECK(rules.maxMoves == 225);
    CHECK(rules.extensions.empty());
}
