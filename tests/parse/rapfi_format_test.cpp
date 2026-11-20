#include <doctest.h>

#include <fstream>
#include <string>
#include <vector>

#include "pygomo/parse/gomocup_parser.hpp"

using namespace pygomo;
using namespace std::chrono_literals;
using pygomo::parse::parseGomocupLine;

namespace {
const InfoEvent* info(const SearchEvent& e) { return std::get_if<InfoEvent>(&e); }
}  // namespace

TEST_CASE("gomo2_parse: Rapfi summary line (Speed | Depth | Eval | Node | Time)") {
    auto e = parseGomocupLine("MESSAGE Speed 333K | Depth 5-6 | Eval -170 | Node 373 | Time 1ms");
    REQUIRE(info(e));
    CHECK(info(e)->depth == 5);
    CHECK(info(e)->score == Score{Score::Kind::Eval, -170});
    CHECK(info(e)->nodes == 373);
    CHECK(info(e)->elapsed == 1ms);

    auto big = parseGomocupLine("MESSAGE Speed 2M | Depth 20-31 | Eval +M5 | Node 12M | Time 3min");
    REQUIRE(info(big));
    CHECK(info(big)->score == Score{Score::Kind::MateIn, 5});
    CHECK(info(big)->nodes == 12'000'000);
    CHECK(info(big)->elapsed == 180'000ms);
}

TEST_CASE("gomo2_parse: per-depth line, with and without [Pondering]") {
    auto e = parseGomocupLine("MESSAGE Depth 4-5 | Eval 0 | Time 12s | E5 B2 A1 B1");
    REQUIRE(info(e));
    CHECK(info(e)->depth == 4);
    CHECK(info(e)->elapsed == 12'000ms);
    CHECK(info(e)->principalVariation == std::vector<Coord>{{4, 4}, {1, 1}, {0, 0}, {1, 0}});

    auto p = parseGomocupLine("MESSAGE [Pondering] Depth 7-9 | Eval -M3 | Time 2h | H8");
    REQUIRE(info(p));
    CHECK(info(p)->score == Score{Score::Kind::MatedIn, 3});
    CHECK(info(p)->elapsed == 7'200'000ms);
    CHECK(info(p)->principalVariation == std::vector<Coord>{{7, 7}});
}

TEST_CASE("gomo2_parse: Bestline, multipv and MCTS lines") {
    auto b = parseGomocupLine("MESSAGE Bestline H7 J9 I9 I10");
    REQUIRE(info(b));
    CHECK(info(b)->principalVariation == std::vector<Coord>{{7, 6}, {9, 8}, {8, 8}, {8, 9}});

    auto m = parseGomocupLine("MESSAGE (2) 34 | 5-6 | E5 B2");
    REQUIRE(info(m));
    CHECK(info(m)->score == Score{Score::Kind::Eval, 34});
    CHECK(info(m)->depth == 5);
    CHECK(info(m)->principalVariation == std::vector<Coord>{{4, 4}, {1, 1}});

    auto mc = parseGomocupLine("MESSAGE (1) 34 (W 50.00, D 10.00, S 0.10) | V 1K | SD 12 | E5");
    REQUIRE(info(mc));
    CHECK(info(mc)->nodes == 1000);
    CHECK(info(mc)->principalVariation == std::vector<Coord>{{4, 4}});

    auto s = parseGomocupLine("MESSAGE Speed 12K | Visit 100 | Time 3ms");
    REQUIRE(info(s));
    CHECK(info(s)->nodes == 100);
    CHECK(info(s)->elapsed == 3ms);
    CHECK_FALSE(info(s)->depth);
}

TEST_CASE("gomo2_parse: the UCI-like form and non-info messages are unchanged") {
    auto u = parseGomocupLine("MESSAGE depth 2-3 ev 0 n 73 n/ms 73 tm 1 pv E5 B2");
    REQUIRE(info(u));
    CHECK(info(u)->depth == 2);
    CHECK(info(u)->elapsed == 1ms);
    CHECK(info(u)->principalVariation == std::vector<Coord>{{4, 4}, {1, 1}});
    for (const char* l : {"MESSAGE OptiTime 873ms | MaxTime 970ms", "MESSAGE Load config from x",
                          "MESSAGE Speed fast | Depth x", "MESSAGE Bestline",
                          "MESSAGE Depth | | "}) {
        INFO(l);
        CHECK(std::holds_alternative<MessageEvent>(parseGomocupLine(l)));
    }
}

TEST_CASE("gomo2_fixtures: real Rapfi transcripts lose no line and record their origin") {
    const std::string dir   = PYGOMO_FIXTURE_DIR;
    int               files = 0;
    for (const char* name :
         {"rapfi-brief-default", "rapfi-normal-partial", "rapfi-ucilike-partial"}) {
        std::ifstream in(dir + "/" + name + ".txt");
        REQUIRE_MESSAGE(in.good(), name);
        ++files;
        std::string line;
        std::string header;
        int engineLines = 0, infos = 0, moves = 0;
        while (std::getline(in, line)) {
            if (!line.empty() && line[0] == '#') { header += line + "\n"; continue; }
            ++engineLines;
            auto e = parseGomocupLine(line);
            if (auto* u = std::get_if<UnparsedEvent>(&e)) CHECK(u->line == line);
            if (auto* m = std::get_if<MessageEvent>(&e)) CHECK("MESSAGE " + m->text == line);
            infos += std::holds_alternative<InfoEvent>(e);
            moves += std::holds_alternative<BestMoveEvent>(e);
        }
        CHECK(header.find("engine: Rapfi") != std::string::npos);
        CHECK(header.find("config:") != std::string::npos);
        CHECK(header.find("sent:") != std::string::npos);
        CHECK(engineLines >= 4);
        CHECK(infos >= 1);
        CHECK(moves == 1);
    }
    CHECK(files == 3);
}
