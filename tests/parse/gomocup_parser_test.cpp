#include <doctest.h>

#include <chrono>
#include <string>

#include "pygomo/parse/gomocup_parser.hpp"

using namespace pygomo;
using namespace std::chrono_literals;
using pygomo::parse::parseGomocupLine;

namespace {
// Line shapes follow Rapfi command/gomocup.cpp, search/searchoutput.cpp, core/iohelper.cpp.
template <class T>
const T* as(const SearchEvent& e) { return std::get_if<T>(&e); }
}  // namespace

TEST_CASE("parse_messages: ERROR becomes ErrorEvent, other text is kept as Unparsed") {
    auto err = parseGomocupLine("ERROR Unsupported board size!");
    REQUIRE(as<ErrorEvent>(err));
    CHECK(as<ErrorEvent>(err)->message == "Unsupported board size!");
    CHECK(*as<ErrorEvent>(parseGomocupLine("ERROR")) == ErrorEvent{""});

    for (const char* l : {"MESSAGE Transposition table cleared.", "DEBUG x", "ERRORS", "UNKNOWN cmd", ""}) {
        auto e = parseGomocupLine(l);
        REQUIRE(as<UnparsedEvent>(e));
        CHECK(as<UnparsedEvent>(e)->line == l);
    }
}

TEST_CASE("parse_move: x,y and SWAP; malformed coordinates are Unparsed") {
    CHECK(*as<BestMoveEvent>(parseGomocupLine("7,7")) == BestMoveEvent{{7, 7}});
    CHECK(*as<BestMoveEvent>(parseGomocupLine("0,14\r")) == BestMoveEvent{{0, 14}});
    CHECK(as<SwapEvent>(parseGomocupLine("SWAP")));
    for (const char* l : {"-1,3", "3,-1", "a,b", "3,", ",3", "3,4,5", "+3,4", "99999999999,1",
                          "7,7 8,8", "3, 4"}) {
        INFO(l);
        CHECK(as<UnparsedEvent>(parseGomocupLine(l)));
    }
}

TEST_CASE("parse_info: UCI-like MESSAGE gives depth, eval, nodes, time and pv") {
    auto e = parseGomocupLine("MESSAGE depth 10-14 ev -34 n 1.25M n/ms 800 tm 900 pv 7,7 8,8 H9");
    REQUIRE(as<InfoEvent>(e));
    const auto& i = *as<InfoEvent>(e);
    CHECK(i.depth == 10);
    CHECK(i.score == Score{Score::Kind::Eval, -34});
    CHECK(i.nodes == 1'250'000);
    CHECK(i.elapsed == 900ms);
    CHECK(i.principalVariation == std::vector<Coord>{{7, 7}, {8, 8}, {7, 8}});
}

TEST_CASE("parse_info: partial lines fill only what is present") {
    auto e = parseGomocupLine("MESSAGE depth 5 ev 12");
    REQUIRE(as<InfoEvent>(e));
    const auto& i = *as<InfoEvent>(e);
    CHECK(i.depth == 5);
    CHECK(i.score == Score{Score::Kind::Eval, 12});
    CHECK_FALSE(i.nodes);
    CHECK_FALSE(i.elapsed);
    CHECK(i.principalVariation.empty());

    auto m = parseGomocupLine("MESSAGE multipv 2 ev 7 pv 1,1");
    REQUIRE(as<InfoEvent>(m));
    CHECK(as<InfoEvent>(m)->principalVariation == std::vector<Coord>{{1, 1}});

    // Truncated or garbled tails never throw and never lose the line.
    for (const char* l : {"MESSAGE depth", "MESSAGE depth x ev y", "MESSAGE depth 5 ev"}) {
        INFO(l);
        CHECK_NOTHROW((void)parseGomocupLine(l));
    }
    CHECK(as<UnparsedEvent>(parseGomocupLine("MESSAGE depth x ev y")));
}

TEST_CASE("parse_unknown: any line is total and keeps its text") {
    for (const char* l : {"OK", "FORBID 0102.", "MESSAGE depth", "\t", "(1) 34 | V 1.2K | SD 12 | H8"}) {
        auto e = parseGomocupLine(l);
        if (auto* u = as<UnparsedEvent>(e)) CHECK(u->line == l);
    }
    CHECK(as<UnparsedEvent>(parseGomocupLine("OK")));
    CHECK(as<UnparsedEvent>(parseGomocupLine("MESSAGE Depth 5 | Eval 3 | Time 1s | H8")));
}
