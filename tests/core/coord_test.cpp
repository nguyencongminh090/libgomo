#include <doctest.h>

#include "pygomo/core/coord.hpp"

using namespace pygomo;

TEST_CASE("Coord defaults to the origin and compares by value") {
    CHECK(Coord{} == Coord{0, 0});
    CHECK(Coord{3, 4} == Coord{3, 4});
    CHECK(Coord{3, 4} != Coord{4, 3});
}

TEST_CASE("Coord orders by x then y") {
    CHECK(Coord{1, 9} < Coord{2, 0});
    CHECK(Coord{2, 0} < Coord{2, 1});
}

TEST_CASE("Side alternates") {
    CHECK(opposite(Side::Black) == Side::White);
    CHECK(opposite(Side::White) == Side::Black);
}

TEST_CASE("Move compares coordinate and side") {
    CHECK(Move{{1, 1}, Side::Black} == Move{{1, 1}, Side::Black});
    CHECK(Move{{1, 1}, Side::Black} != Move{{1, 1}, Side::White});
}
