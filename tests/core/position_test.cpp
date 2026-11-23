#include <doctest.h>

#include <algorithm>
#include <string>

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
    for (auto [w, h] : {std::pair{0, 15},
                        {15, 0},
                        {-1, 15},
                        {15, -3},
                        {Position::kMaxSide + 1, 15},
                        {15, Position::kMaxSide + 1}}) {
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

TEST_CASE("ext06_layout: play on a blocked cell fails with Blocked and changes nothing") {
    BoardLayout layout;
    layout.blocked = {{2, 2}, {0, 4}};
    auto p         = Position::create(5, 5, layout).value();
    REQUIRE(p.play({1, 1}).has_value());
    const Position before = p;

    for (Coord c : {Coord{2, 2}, Coord{0, 4}}) {
        auto r = p.play(c);
        REQUIRE_FALSE(r.has_value());
        CHECK(r.error() == PositionError::Blocked);
        CHECK(p.blocked(c));
        // A blocked cell holds no stone.
        CHECK_FALSE(p.occupied(c));
    }
    CHECK(p == before);
    CHECK(p.moves().size() == 1);
    CHECK(p.sideToMove() == Side::White);
    CHECK_FALSE(p.blocked({1, 1}));
    CHECK_FALSE(p.blocked({-1, 0}));
}

TEST_CASE("ext06_layout: play checks OutOfBounds before Blocked") {
    auto p = Position::create(3, 3, BoardLayout{.blocked = {{0, 0}}}).value();
    auto r = p.play({3, 0});
    REQUIRE_FALSE(r.has_value());
    CHECK(r.error() == PositionError::OutOfBounds);
}

TEST_CASE("ext06_layout: moves and sideToMove are unchanged by a layout") {
    BoardLayout layout;
    layout.torus      = true;
    layout.blocked    = {{0, 0}};
    layout.portals    = {{{1, 1}, {13, 13}}};
    layout.extensions = {{"note", Value{std::string{"x"}}}};
    auto plain        = Position::create(15, 15).value();
    auto shaped       = Position::create(15, 15, layout).value();
    for (Coord c : {Coord{7, 7}, Coord{8, 7}, Coord{1, 1}}) {
        REQUIRE(plain.play(c).has_value());
        REQUIRE(shaped.play(c).has_value());
    }
    CHECK(std::ranges::equal(plain.moves(), shaped.moves()));
    CHECK(shaped.sideToMove() == plain.sideToMove());
    CHECK(shaped.layout() == layout);
}

TEST_CASE("ext06_layout: layouts take part in equality") {
    auto plain  = Position::create(15, 15).value();
    auto walled = Position::create(15, 15, BoardLayout{.blocked = {{0, 0}}}).value();
    auto torus  = Position::create(15, 15, BoardLayout{.torus = true}).value();
    CHECK(plain != walled);
    CHECK(plain != torus);
    CHECK(walled != torus);
    REQUIRE(plain.play({7, 7}).has_value());
    REQUIRE(walled.play({7, 7}).has_value());
    CHECK(plain != walled);
    auto walledAgain = Position::create(15, 15, BoardLayout{.blocked = {{0, 0}}}).value();
    REQUIRE(walledAgain.play({7, 7}).has_value());
    CHECK(walled == walledAgain);
}

TEST_CASE("ext06_layout: create rejects an off-board blocked or portal cell") {
    for (Coord off : {Coord{-1, 0}, Coord{0, -1}, Coord{5, 0}, Coord{0, 4}}) {
        auto wall = Position::create(5, 4, BoardLayout{.blocked = {{1, 1}, off}});
        REQUIRE_FALSE(wall.has_value());
        CHECK(wall.error() == PositionError::OutOfBounds);

        auto first = Position::create(5, 4, BoardLayout{.portals = {{off, {1, 1}}}});
        REQUIRE_FALSE(first.has_value());
        CHECK(first.error() == PositionError::OutOfBounds);

        auto second = Position::create(5, 4, BoardLayout{.portals = {{{1, 1}, off}}});
        REQUIRE_FALSE(second.has_value());
        CHECK(second.error() == PositionError::OutOfBounds);
    }
    auto bad = Position::create(0, 4, BoardLayout{.blocked = {{9, 9}}});
    REQUIRE_FALSE(bad.has_value());
    CHECK(bad.error() == PositionError::InvalidBoardSize);
    CHECK(Position::create(5, 4, BoardLayout{.blocked = {{4, 3}}, .portals = {{{0, 0}, {4, 3}}}})
              .has_value());
}

TEST_CASE("ext06_layout: the default layout keeps the old behaviour") {
    auto p = Position::create(15, 15).value();
    CHECK(p.layout() == BoardLayout{});
    CHECK(p == Position::create(15, 15, BoardLayout{}).value());
    for (int y = 0; y < 15; ++y) {
        for (int x = 0; x < 15; ++x) CHECK_FALSE(p.blocked({x, y}));
    }
    REQUIRE(p.play({0, 0}).has_value());
    auto again = p.play({0, 0});
    REQUIRE_FALSE(again.has_value());
    CHECK(again.error() == PositionError::Occupied);
}
