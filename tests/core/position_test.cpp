#include <doctest.h>

#include "pygomo/core/position.hpp"

using namespace pygomo;

TEST_CASE("Position::create accepts square and rectangular boards") {
    auto square = Position::create(15, 15);
    REQUIRE(square.has_value());
    CHECK(square->width() == 15);
    CHECK(square->height() == 15);

    auto rect = Position::create(20, 12);
    REQUIRE(rect.has_value());
    CHECK(rect->width() == 20);
    CHECK(rect->height() == 12);
}

TEST_CASE("Position::create rejects sizes outside 1..kMaxSide") {
    for (auto [w, h] : {std::pair{0, 15}, {15, 0}, {-1, 15}, {15, -3},
                        {Position::kMaxSide + 1, 15}, {15, Position::kMaxSide + 1}}) {
        auto p = Position::create(w, h);
        REQUIRE_FALSE(p.has_value());
        CHECK(p.error() == PositionError::InvalidBoardSize);
    }
    CHECK(Position::create(1, 1).has_value());
    CHECK(Position::create(Position::kMaxSide, Position::kMaxSide).has_value());
}

TEST_CASE("A new position is empty with Black to move") {
    auto p = Position::create(15, 15).value();
    CHECK(p.moves().empty());
    CHECK(p.sideToMove() == Side::Black);
    CHECK_FALSE(p.occupied({7, 7}));
}

TEST_CASE("play records moves in order and alternates the side") {
    auto p = Position::create(15, 15).value();
    REQUIRE(p.play({7, 7}).has_value());
    REQUIRE(p.play({8, 7}).has_value());
    REQUIRE(p.play({7, 8}).has_value());

    REQUIRE(p.moves().size() == 3);
    CHECK(p.moves()[0] == Move{{7, 7}, Side::Black});
    CHECK(p.moves()[1] == Move{{8, 7}, Side::White});
    CHECK(p.moves()[2] == Move{{7, 8}, Side::Black});
    CHECK(p.sideToMove() == Side::White);
    CHECK(p.occupied({8, 7}));
}

TEST_CASE("play rejects out-of-bounds and occupied points and leaves the position unchanged") {
    auto p = Position::create(5, 3).value();
    REQUIRE(p.play({4, 2}).has_value());
    const Position before = p;

    for (Coord c : {Coord{-1, 0}, Coord{0, -1}, Coord{5, 0}, Coord{0, 3}}) {
        auto r = p.play(c);
        REQUIRE_FALSE(r.has_value());
        CHECK(r.error() == PositionError::OutOfBounds);
    }
    auto occupied = p.play({4, 2});
    REQUIRE_FALSE(occupied.has_value());
    CHECK(occupied.error() == PositionError::Occupied);

    CHECK(p == before);
    CHECK(p.sideToMove() == Side::White);
}

TEST_CASE("Positions compare by size and move list") {
    auto a = Position::create(15, 15).value();
    auto b = Position::create(15, 15).value();
    CHECK(a == b);
    REQUIRE(a.play({1, 1}).has_value());
    CHECK(a != b);
    REQUIRE(b.play({1, 1}).has_value());
    CHECK(a == b);
    CHECK(Position::create(15, 14).value() != Position::create(15, 15).value());
}
