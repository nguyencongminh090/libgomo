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

TEST_CASE("gtp_analyze: kata-analyze fixture, first entry only") {
    auto e = parseGtpAnalyzeLine(fixtureLine("kata_analyze.txt"), 15, 15);
    auto* info = std::get_if<InfoEvent>(&e);
    REQUIRE(info);
    CHECK(info->nodes == 120);
    CHECK_FALSE(info->score);
    CHECK_FALSE(info->depth);
    REQUIRE(info->principalVariation.size() == 3);
    CHECK(info->principalVariation[0] == Coord{7, 7});   // H8
    CHECK(info->principalVariation[1] == Coord{6, 8});   // G7
    CHECK(info->principalVariation[2] == Coord{8, 6});   // J9
}

TEST_CASE("gtp_analyze: lz-analyze fixture") {
    auto e = parseGtpAnalyzeLine(fixtureLine("lz_analyze.txt"), 15, 15);
    auto* info = std::get_if<InfoEvent>(&e);
    REQUIRE(info);
    CHECK(info->nodes == 33);
    REQUIRE(info->principalVariation.size() == 3);
    CHECK(info->principalVariation.front() == Coord{7, 7});
}

TEST_CASE("gtp_analyze: PV stops at pass; bad lines stay unparsed") {
    auto e = parseGtpAnalyzeLine("info move H8 visits 5 pv H8 pass G7", 15, 15);
    auto* info = std::get_if<InfoEvent>(&e);
    REQUIRE(info);
    CHECK(info->principalVariation.size() == 1);

    for (std::string_view bad : {"", "hello", "info", "info move H8 pv H8 Z99",
                                 "info visits many pv H8", "info move H8 pv I5"}) {
        auto u = parseGtpAnalyzeLine(bad, 15, 15);
        auto* un = std::get_if<UnparsedEvent>(&u);
        REQUIRE(un);
        CHECK(un->line == bad);
    }
}

TEST_CASE("gtp_total: no input makes a parser throw or read out of bounds") {
    std::mt19937 rng(12345);
    const std::string alphabet = "=?()0123456789,ABCabcIiJjzZ-+ \t\r\n#infomvspvisitspass";
    std::uniform_int_distribution<std::size_t> pick(0, alphabet.size() - 1);
    std::uniform_int_distribution<int> len(0, 80);
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
