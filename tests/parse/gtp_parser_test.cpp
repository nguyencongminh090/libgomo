#include <doctest.h>

#include <fstream>
#include <random>
#include <string>

#include "pygomo/parse/gtp_parser.hpp"

using namespace pygomo;
using namespace pygomo::parse;

namespace {

std::string fixtureLine(const std::string& name) {
    std::ifstream in(std::string{PYGOMO_GTP_FIXTURE_DIR} + "/" + name);
    REQUIRE(in.good());
    std::string line;
    while (std::getline(in, line))
        if (!line.empty() && line.front() != '#') return line;
    FAIL("fixture has no data line: " << name);
    return {};
}

std::optional<Coord> coordOf(std::string_view text, int w, int h) {
    auto v = parseGtpVertex(text, w, h);
    if (!v) return std::nullopt;
    if (auto* c = std::get_if<Coord>(&*v)) return *c;
    return std::nullopt;
}

}  // namespace

TEST_CASE("gtp_header: ok, error, id and body") {
    auto a = parseGtpResponseHeader("=12 hello");
    REQUIRE(a);
    CHECK(a->ok);
    CHECK(a->id == 12);
    CHECK(a->body == "hello");

    auto b = parseGtpResponseHeader("?7 bad move");
    REQUIRE(b);
    CHECK_FALSE(b->ok);
    CHECK(b->id == 7);
    CHECK(b->body == "bad move");

    auto c = parseGtpResponseHeader("= ");
    REQUIRE(c);
    CHECK(c->ok);
    CHECK_FALSE(c->id);
    CHECK(c->body.empty());

    auto d = parseGtpResponseHeader("=");
    REQUIRE(d);
    CHECK_FALSE(d->id);
    CHECK(d->body.empty());

    auto e = parseGtpResponseHeader("= H8\r");
    REQUIRE(e);
    CHECK(e->body == "H8");
}

TEST_CASE("gtp_header: lines that are not a response start") {
    CHECK_FALSE(parseGtpResponseHeader(""));
    CHECK_FALSE(parseGtpResponseHeader("info move H8"));
    CHECK_FALSE(parseGtpResponseHeader(" =1"));
    CHECK_FALSE(parseGtpResponseHeader("=12345678901 too long id"));
}

TEST_CASE("gtp_vertex: letters skip I and rows count from the bottom") {
    CHECK(coordOf("A1", 15, 15) == Coord{0, 14});
    CHECK(coordOf("A15", 15, 15) == Coord{0, 0});
    CHECK(coordOf("O15", 15, 15) == Coord{13, 0});
    CHECK(coordOf("P15", 15, 15) == Coord{14, 0});  // last of 15 columns: A-H, J-P
    CHECK(coordOf("H8", 15, 15) == Coord{7, 7});
    CHECK(coordOf("J9", 15, 15) == Coord{8, 6});
    CHECK(coordOf("j10", 19, 19) == Coord{8, 9});
    CHECK(coordOf("D4", 19, 19) == Coord{3, 15});
    CHECK_FALSE(parseGtpVertex("I5", 15, 15));
    CHECK_FALSE(parseGtpVertex("i5", 19, 19));
}

TEST_CASE("gtp_vertex: pass, resign, machine form, two letters") {
    CHECK(parseGtpVertex("pass", 15, 15) == GtpVertex{GtpPass{}});
    CHECK(parseGtpVertex("PASS", 15, 15) == GtpVertex{GtpPass{}});
    CHECK(parseGtpVertex("resign", 15, 15) == GtpVertex{GtpResign{}});
    CHECK(coordOf("(3,4)", 15, 15) == Coord{3, 4});
    CHECK_FALSE(parseGtpVertex("(15,0)", 15, 15));
    CHECK_FALSE(parseGtpVertex("(1,2", 15, 15));
    // AA is the first two-letter column: (0 + 1) * 25 + 0 = 25.
    CHECK(coordOf("AA3", 30, 30) == Coord{25, 27});
    CHECK_FALSE(parseGtpVertex("AA3", 19, 19));
}

TEST_CASE("gtp_vertex: out of board and malformed text") {
    CHECK_FALSE(parseGtpVertex("A0", 15, 15));
    CHECK_FALSE(parseGtpVertex("A16", 15, 15));
    CHECK_FALSE(parseGtpVertex("Q1", 15, 15));
    CHECK_FALSE(parseGtpVertex("H", 15, 15));
    CHECK_FALSE(parseGtpVertex("H8x", 15, 15));
    CHECK_FALSE(parseGtpVertex("H-8", 15, 15));
    CHECK_FALSE(parseGtpVertex("H8", 0, 15));
}

TEST_CASE("gtp_vertex: format and parse round-trip on every point") {
    for (int size : {15, 19, 30}) {
        for (int y = 0; y < size; ++y) {
            for (int x = 0; x < size; ++x) {
                auto text = formatGtpVertex(Coord{x, y}, size, size);
                REQUIRE(text);
                CHECK(coordOf(*text, size, size) == Coord{x, y});
            }
        }
    }
    CHECK(formatGtpVertex(Coord{0, 14}, 15, 15) == "A1");
    CHECK(formatGtpVertex(Coord{7, 7}, 15, 15) == "H8");
    CHECK_FALSE(formatGtpVertex(Coord{15, 0}, 15, 15));
    CHECK_FALSE(formatGtpVertex(Coord{-1, 0}, 15, 15));
}

TEST_CASE("gtp_vertex: rectangular board") {
    CHECK(coordOf("A1", 20, 10) == Coord{0, 9});
    CHECK(coordOf("T10", 20, 10) == Coord{18, 0});
    CHECK_FALSE(parseGtpVertex("A11", 20, 10));
}

TEST_CASE("gtp_analyze: kata-analyze fixture") {
    auto  e    = parseGtpAnalyzeLine(fixtureLine("kata_analyze.txt"), 15, 15);
    auto* info = std::get_if<InfoEvent>(&e);
    REQUIRE(info);
    CHECK_FALSE(info->nodes);
    REQUIRE_FALSE(info->candidates.empty());
    CHECK(info->candidates[0].visits == 120);
    CHECK_FALSE(info->score);
    CHECK_FALSE(info->depth);
    REQUIRE(info->principalVariation.size() == 3);
    CHECK(info->principalVariation[0] == Coord{7, 7});  // H8
    CHECK(info->principalVariation[1] == Coord{6, 8});  // G7
    CHECK(info->principalVariation[2] == Coord{8, 6});  // J9
}

TEST_CASE("gtp_analyze: lz-analyze fixture") {
    auto  e    = parseGtpAnalyzeLine(fixtureLine("lz_analyze.txt"), 15, 15);
    auto* info = std::get_if<InfoEvent>(&e);
    REQUIRE(info);
    CHECK_FALSE(info->nodes);
    REQUIRE_FALSE(info->candidates.empty());
    CHECK(info->candidates[0].visits == 33);
    REQUIRE(info->principalVariation.size() == 3);
    CHECK(info->principalVariation.front() == Coord{7, 7});
}

TEST_CASE("gtp_analyze: PV stops at pass; bad lines stay unparsed") {
    auto  e    = parseGtpAnalyzeLine("info move H8 visits 5 pv H8 pass G7", 15, 15);
    auto* info = std::get_if<InfoEvent>(&e);
    REQUIRE(info);
    CHECK(info->principalVariation.size() == 1);

    for (std::string_view bad : {"", "hello", "info", "info move H8 pv H8 Z99",
                                 "info visits many pv H8", "info move H8 pv I5"}) {
        auto  u  = parseGtpAnalyzeLine(bad, 15, 15);
        auto* un = std::get_if<UnparsedEvent>(&u);
        REQUIRE(un);
        CHECK(un->line == bad);
    }
}

TEST_CASE("gtp_total: no input makes a parser throw or read out of bounds") {
    std::mt19937      rng(12345);
    const std::string alphabet = "=?()0123456789,ABCabcIiJjzZ-+ \t\r\n#infomvspvisitspass";
    std::uniform_int_distribution<std::size_t> pick(0, alphabet.size() - 1);
    std::uniform_int_distribution<int>         len(0, 80);
    for (int round = 0; round < 3000; ++round) {
        std::string s;
        for (int n = len(rng); n > 0; --n) s += alphabet[pick(rng)];
        CHECK_NOTHROW((void)parseGtpResponseHeader(s));
        CHECK_NOTHROW((void)parseGtpVertex(s, 19, 19));
        CHECK_NOTHROW((void)parseGtpAnalyzeLine(s, 19, 19));
        CHECK_NOTHROW((void)parseGtpAnalyzeLine("info pv " + s, 19, 19));
    }
    std::string huge(200000, '9');
    CHECK_NOTHROW((void)parseGtpResponseHeader("=" + huge));
    CHECK_NOTHROW((void)parseGtpVertex("A" + huge, 19, 19));
    CHECK_NOTHROW((void)parseGtpVertex("(" + huge + "," + huge + ")", 19, 19));
    CHECK_NOTHROW((void)parseGtpAnalyzeLine("info visits " + huge, 19, 19));
    CHECK_NOTHROW((void)parseGtpVertex("A1", -5, 100000));
    CHECK_NOTHROW((void)formatGtpVertex(Coord{1 << 30, -(1 << 30)}, 19, 19));
    CHECK_FALSE(parseGtpVertex("A1", 1 << 30, 1 << 30));
}

TEST_CASE("gtp_header: a marker glued to other text is not a header") {
    CHECK_FALSE(parseGtpResponseHeader("=== Loaded ==="));
    CHECK_FALSE(parseGtpResponseHeader("??x"));
    CHECK_FALSE(parseGtpResponseHeader("=7K10"));
    CHECK_FALSE(parseGtpResponseHeader("?3x"));
    CHECK(parseGtpResponseHeader("="));
    CHECK(parseGtpResponseHeader("=7"));
    CHECK(parseGtpResponseHeader("? msg"));
}

TEST_CASE("gtp7_probe: single entry and two entries produce different InfoEvents") {
    const std::string_view oneLine = "info move H8 visits 120 pv H8";
    const std::string_view twoLine = "info move H8 visits 120 pv H8 info move G8 visits 50 pv G8";

    auto  e1  = parseGtpAnalyzeLine(oneLine, 15, 15);
    auto  e2  = parseGtpAnalyzeLine(twoLine, 15, 15);
    auto* one = std::get_if<InfoEvent>(&e1);
    auto* two = std::get_if<InfoEvent>(&e2);
    REQUIRE(one);
    REQUIRE(two);
    CHECK(*one != *two);

    CHECK_FALSE(one->nodes);
    CHECK_FALSE(one->score);
    CHECK(one->principalVariation == std::vector<Coord>{{7, 7}});
    REQUIRE(one->candidates.size() == 1);
    CHECK_FALSE(one->candidates[0].rank);
    CHECK(one->candidates[0].move == Coord{7, 7});
    CHECK(one->candidates[0].visits == 120);
    CHECK_FALSE(one->candidates[0].winrate);
    CHECK_FALSE(one->candidates[0].drawrate);
    CHECK_FALSE(one->candidates[0].score);
    CHECK(one->candidates[0].principalVariation == std::vector<Coord>{{7, 7}});

    CHECK_FALSE(two->nodes);
    CHECK_FALSE(two->score);
    CHECK(two->principalVariation == std::vector<Coord>{{7, 7}});
    REQUIRE(two->candidates.size() == 2);
    CHECK_FALSE(two->candidates[0].rank);
    CHECK(two->candidates[0].move == Coord{7, 7});
    CHECK(two->candidates[0].visits == 120);
    CHECK(two->candidates[0].principalVariation == std::vector<Coord>{{7, 7}});
    CHECK_FALSE(two->candidates[1].rank);
    CHECK(two->candidates[1].move == Coord{6, 7});  // G8
    CHECK(two->candidates[1].visits == 50);
    CHECK(two->candidates[1].principalVariation == std::vector<Coord>{{6, 7}});
}

TEST_CASE("gtp7_fixtures: kata-analyze reads all candidates with rank, visits, winrate, PV") {
    auto  e    = parseGtpAnalyzeLine(fixtureLine("kata_analyze.txt"), 15, 15);
    auto* info = std::get_if<InfoEvent>(&e);
    REQUIRE(info);
    CHECK_FALSE(info->nodes);
    CHECK_FALSE(info->score);
    CHECK(info->principalVariation == std::vector<Coord>{{7, 7}, {6, 8}, {8, 6}});

    REQUIRE(info->candidates.size() == 2);
    const auto& first = info->candidates[0];
    CHECK(first.rank == 1);
    CHECK(first.move == Coord{7, 7});
    CHECK(first.visits == 120);
    REQUIRE(first.winrate);
    CHECK(*first.winrate == doctest::Approx(0.56));
    CHECK_FALSE(first.drawrate);
    CHECK_FALSE(first.score);
    CHECK(first.principalVariation == std::vector<Coord>{{7, 7}, {6, 8}, {8, 6}});

    const auto& second = info->candidates[1];
    CHECK(second.rank == 2);
    CHECK(second.move == Coord{6, 8});
    CHECK(second.visits == 40);
    REQUIRE(second.winrate);
    CHECK(*second.winrate == doctest::Approx(0.51));
    CHECK_FALSE(second.drawrate);
    CHECK_FALSE(second.score);
    CHECK(second.principalVariation == std::vector<Coord>{{6, 8}, {7, 7}});
}

TEST_CASE("gtp7_fixtures: lz-analyze reads all candidates with scaled winrate") {
    auto  e    = parseGtpAnalyzeLine(fixtureLine("lz_analyze.txt"), 15, 15);
    auto* info = std::get_if<InfoEvent>(&e);
    REQUIRE(info);
    CHECK_FALSE(info->nodes);
    CHECK_FALSE(info->score);
    CHECK(info->principalVariation == std::vector<Coord>{{7, 7}, {8, 6}, {6, 8}});

    REQUIRE(info->candidates.size() == 2);
    const auto& first = info->candidates[0];
    CHECK(first.rank == 1);
    CHECK(first.move == Coord{7, 7});
    CHECK(first.visits == 33);
    REQUIRE(first.winrate);
    CHECK(*first.winrate == doctest::Approx(0.521));
    CHECK_FALSE(first.drawrate);
    CHECK_FALSE(first.score);
    CHECK(first.principalVariation == std::vector<Coord>{{7, 7}, {8, 6}, {6, 8}});

    const auto& second = info->candidates[1];
    CHECK(second.rank == 2);
    CHECK(second.move == Coord{9, 5});  // K10
    CHECK(second.visits == 5);
    REQUIRE(second.winrate);
    CHECK(*second.winrate == doctest::Approx(0.48));
    CHECK_FALSE(second.drawrate);
    CHECK_FALSE(second.score);
    CHECK(second.principalVariation == std::vector<Coord>{{9, 5}});
}

TEST_CASE("gtp7_candidates: summary PV follows rank 1; non-rank-1 leaves summary PV empty") {
    const std::string_view swapped =
        "info move G8 visits 40 order 1 pv G8 info move H8 visits 120 order 0 pv H8 J9";
    auto  e1 = parseGtpAnalyzeLine(swapped, 15, 15);
    auto* i1 = std::get_if<InfoEvent>(&e1);
    REQUIRE(i1);
    REQUIRE(i1->candidates.size() == 2);
    CHECK(i1->candidates[0].rank == 2);
    CHECK(i1->candidates[1].rank == 1);
    CHECK(i1->principalVariation == std::vector<Coord>{{7, 7}, {8, 6}});

    auto  e2 = parseGtpAnalyzeLine("info move G8 visits 40 order 1 pv G8", 15, 15);
    auto* i2 = std::get_if<InfoEvent>(&e2);
    REQUIRE(i2);
    REQUIRE(i2->candidates.size() == 1);
    CHECK(i2->candidates[0].rank == 2);
    CHECK(i2->principalVariation.empty());
}

TEST_CASE("gtp7_candidates: move pass, skipped keys, pvEdgeVisits and winrate bounds") {
    const std::string_view passLine =
        "info move pass visits 10 utility 0.0 winrate 0.5 scoreMean 25 scoreLead 25 "
        "isSymmetryOf H8 order 0 pv H8 pvEdgeVisits 10";
    auto  e    = parseGtpAnalyzeLine(passLine, 15, 15);
    auto* info = std::get_if<InfoEvent>(&e);
    REQUIRE(info);
    REQUIRE(info->candidates.size() == 1);
    const auto& c = info->candidates[0];
    CHECK(c.rank == 1);
    CHECK_FALSE(c.move);
    CHECK(c.visits == 10);
    REQUIRE(c.winrate);
    CHECK(*c.winrate == doctest::Approx(0.5));
    CHECK_FALSE(c.drawrate);
    CHECK_FALSE(c.score);
    CHECK(c.principalVariation == std::vector<Coord>{{7, 7}});

    for (std::string_view outOfRange : {"info move H8 visits 10 utility 0.1 winrate 1.05 pv H8",
                                        "info move H8 visits 10 utility 0.1 winrate -0.01 pv H8",
                                        "info move H8 visits 10 winrate 10001 pv H8",
                                        "info move H8 visits 10 winrate -1 pv H8"}) {
        INFO(outOfRange);
        auto  ev = parseGtpAnalyzeLine(outOfRange, 15, 15);
        auto* in = std::get_if<InfoEvent>(&ev);
        REQUIRE(in);
        REQUIRE(in->candidates.size() == 1);
        CHECK_FALSE(in->candidates[0].winrate);
    }

    auto bounds = parseGtpAnalyzeLine(
        "info move H8 visits 10 winrate 0 pv H8 info move G8 visits 5 winrate 10000 pv G8", 15, 15);
    auto* bi = std::get_if<InfoEvent>(&bounds);
    REQUIRE(bi);
    REQUIRE(bi->candidates.size() == 2);
    REQUIRE(bi->candidates[0].winrate);
    CHECK(*bi->candidates[0].winrate == doctest::Approx(0.0));
    REQUIRE(bi->candidates[1].winrate);
    CHECK(*bi->candidates[1].winrate == doctest::Approx(1.0));
}

TEST_CASE("gtp7_robust: broken later entry keeps earlier candidates and stops reading") {
    const std::string_view badSecond =
        "info move H8 visits 120 order 0 pv H8 info move G8 visits bad pv G8 "
        "info move F8 visits 10 order 2 pv F8";
    auto  e1 = parseGtpAnalyzeLine(badSecond, 15, 15);
    auto* i1 = std::get_if<InfoEvent>(&e1);
    REQUIRE(i1);
    REQUIRE(i1->candidates.size() == 1);
    CHECK(i1->candidates[0].move == Coord{7, 7});
    CHECK(i1->candidates[0].visits == 120);
    CHECK(i1->principalVariation == std::vector<Coord>{{7, 7}});

    const std::string_view badThird =
        "info move H8 visits 120 pv H8 info move G8 visits 50 pv G8 info move Z99 pv Z99";
    auto  e2 = parseGtpAnalyzeLine(badThird, 15, 15);
    auto* i2 = std::get_if<InfoEvent>(&e2);
    REQUIRE(i2);
    REQUIRE(i2->candidates.size() == 2);
    CHECK(i2->candidates[0].move == Coord{7, 7});
    CHECK(i2->candidates[1].move == Coord{6, 7});

    auto  e3 = parseGtpAnalyzeLine("info move H8 visits 120 pv H8 info", 15, 15);
    auto* i3 = std::get_if<InfoEvent>(&e3);
    REQUIRE(i3);
    REQUIRE(i3->candidates.size() == 1);
    CHECK(i3->candidates[0].move == Coord{7, 7});
}

TEST_CASE("gtp7_robust: broken first entry and non-info lines give UnparsedEvent") {
    for (std::string_view bad : {"", "not an info line", "info",
                                 "info move Z99 visits 10 pv H8 info move G8 visits 5 pv G8",
                                 "info move H8 visits bad pv H8 info move G8 visits 5 pv G8",
                                 "info move H8 order -1 pv H8", "info move H8 winrate nope pv H8",
                                 "info move H8 visits -3 pv H8"}) {
        INFO(bad);
        auto  e  = parseGtpAnalyzeLine(bad, 15, 15);
        auto* un = std::get_if<UnparsedEvent>(&e);
        REQUIRE(un);
        CHECK(un->line == bad);
    }
}
