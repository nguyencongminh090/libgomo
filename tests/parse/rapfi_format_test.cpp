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
    CHECK(info(m)->depth == 5);
    CHECK_FALSE(info(m)->score);
    CHECK(info(m)->principalVariation.empty());
    REQUIRE(info(m)->candidates.size() == 1);
    CHECK(info(m)->candidates[0].rank == 2);
    CHECK(info(m)->candidates[0].move == Coord{4, 4});
    CHECK(info(m)->candidates[0].score == Score{Score::Kind::Eval, 34});
    CHECK(info(m)->candidates[0].principalVariation == std::vector<Coord>{{4, 4}, {1, 1}});

    auto mc = parseGomocupLine("MESSAGE (1) 34 (W 50.00, D 10.00, S 0.10) | V 1K | SD 12 | E5");
    REQUIRE(info(mc));
    CHECK_FALSE(info(mc)->nodes);
    REQUIRE(info(mc)->candidates.size() == 1);
    CHECK(info(mc)->candidates[0].visits == 1000);
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

TEST_CASE("gomo7_ranks: multipv 1 and multipv 2 probes produce distinct InfoEvents") {
    auto r1 = parseGomocupLine("MESSAGE multipv 1 ev 7 pv 1,1");
    auto r2 = parseGomocupLine("MESSAGE multipv 2 ev 7 pv 1,1");
    REQUIRE(info(r1));
    REQUIRE(info(r2));
    CHECK(*info(r1) != *info(r2));

    REQUIRE(info(r1)->candidates.size() == 1);
    const auto& c1 = info(r1)->candidates[0];
    CHECK(c1.rank == 1);
    CHECK(c1.move == Coord{1, 1});
    CHECK(c1.score == Score{Score::Kind::Eval, 7});
    CHECK(c1.principalVariation == std::vector<Coord>{{1, 1}});
    CHECK_FALSE(c1.visits);
    CHECK_FALSE(c1.winrate);
    CHECK_FALSE(c1.drawrate);
    CHECK(info(r1)->score == Score{Score::Kind::Eval, 7});
    CHECK(info(r1)->principalVariation == std::vector<Coord>{{1, 1}});
    CHECK_FALSE(info(r1)->nodes);

    REQUIRE(info(r2)->candidates.size() == 1);
    const auto& c2 = info(r2)->candidates[0];
    CHECK(c2.rank == 2);
    CHECK(c2.move == Coord{1, 1});
    CHECK(c2.score == Score{Score::Kind::Eval, 7});
    CHECK(c2.principalVariation == std::vector<Coord>{{1, 1}});
    CHECK_FALSE(info(r2)->score);
    CHECK(info(r2)->principalVariation.empty());
    CHECK_FALSE(info(r2)->nodes);
}

TEST_CASE("gomo7_forms: all four Rapfi ranked forms map candidate and summary fields") {
    // Form 1: normal AB `(N) v | d-s | pv`, with and without [Pondering].
    auto n1 = parseGomocupLine("MESSAGE [Pondering] (1) +M5 | 10-12 | H8 G8");
    REQUIRE(info(n1));
    CHECK(info(n1)->depth == 10);
    CHECK(info(n1)->score == Score{Score::Kind::MateIn, 5});
    CHECK(info(n1)->principalVariation == std::vector<Coord>{{7, 7}, {6, 7}});
    CHECK_FALSE(info(n1)->nodes);
    REQUIRE(info(n1)->candidates.size() == 1);
    CHECK(info(n1)->candidates[0].rank == 1);
    CHECK(info(n1)->candidates[0].move == Coord{7, 7});
    CHECK(info(n1)->candidates[0].score == Score{Score::Kind::MateIn, 5});
    CHECK(info(n1)->candidates[0].principalVariation == std::vector<Coord>{{7, 7}, {6, 7}});
    CHECK_FALSE(info(n1)->candidates[0].visits);
    CHECK_FALSE(info(n1)->candidates[0].winrate);
    CHECK_FALSE(info(n1)->candidates[0].drawrate);

    auto n2 = parseGomocupLine("MESSAGE (2) 34 | 5-6 | E5 B2");
    REQUIRE(info(n2));
    CHECK(info(n2)->depth == 5);
    CHECK_FALSE(info(n2)->score);
    CHECK(info(n2)->principalVariation.empty());
    CHECK_FALSE(info(n2)->nodes);
    REQUIRE(info(n2)->candidates.size() == 1);
    CHECK(info(n2)->candidates[0].rank == 2);
    CHECK(info(n2)->candidates[0].move == Coord{4, 4});
    CHECK(info(n2)->candidates[0].score == Score{Score::Kind::Eval, 34});
    CHECK(info(n2)->candidates[0].principalVariation == std::vector<Coord>{{4, 4}, {1, 1}});

    // Form 2: normal MCTS `(N) v (W w, D d, S s) | V n | SD sd | pv`.
    auto m1 = parseGomocupLine(
        "MESSAGE (1) 34 (W 50.00, D 10.00, S 0.10) | V 1K | SD 12 | E5 B2");
    REQUIRE(info(m1));
    CHECK_FALSE(info(m1)->nodes);
    CHECK_FALSE(info(m1)->depth);
    CHECK(info(m1)->score == Score{Score::Kind::Eval, 34});
    CHECK(info(m1)->principalVariation == std::vector<Coord>{{4, 4}, {1, 1}});
    REQUIRE(info(m1)->candidates.size() == 1);
    CHECK(info(m1)->candidates[0].rank == 1);
    CHECK(info(m1)->candidates[0].move == Coord{4, 4});
    CHECK(info(m1)->candidates[0].score == Score{Score::Kind::Eval, 34});
    CHECK(info(m1)->candidates[0].visits == 1000);
    REQUIRE(info(m1)->candidates[0].winrate);
    REQUIRE(info(m1)->candidates[0].drawrate);
    CHECK(*info(m1)->candidates[0].winrate == doctest::Approx(0.5));
    CHECK(*info(m1)->candidates[0].drawrate == doctest::Approx(0.1));
    CHECK(info(m1)->candidates[0].principalVariation == std::vector<Coord>{{4, 4}, {1, 1}});

    auto m2 = parseGomocupLine(
        "MESSAGE (2) -12 (W 45.25, D 5.50, S 0.20) | V 2.5M | SD 8 | H8");
    REQUIRE(info(m2));
    CHECK_FALSE(info(m2)->nodes);
    CHECK_FALSE(info(m2)->score);
    CHECK(info(m2)->principalVariation.empty());
    REQUIRE(info(m2)->candidates.size() == 1);
    CHECK(info(m2)->candidates[0].rank == 2);
    CHECK(info(m2)->candidates[0].move == Coord{7, 7});
    CHECK(info(m2)->candidates[0].score == Score{Score::Kind::Eval, -12});
    CHECK(info(m2)->candidates[0].visits == 2'500'000);
    REQUIRE(info(m2)->candidates[0].winrate);
    REQUIRE(info(m2)->candidates[0].drawrate);
    CHECK(*info(m2)->candidates[0].winrate == doctest::Approx(0.4525));
    CHECK(*info(m2)->candidates[0].drawrate == doctest::Approx(0.055));
    CHECK(info(m2)->candidates[0].principalVariation == std::vector<Coord>{{7, 7}});

    // Form 3: UCI-like AB `depth d-s multipv N ev v n ... tm t pv ...`.
    auto u1 = parseGomocupLine(
        "MESSAGE depth 10-14 multipv 1 ev -34 n 1.25M n/ms 800 tm 900 pv 7,7 8,8");
    REQUIRE(info(u1));
    CHECK(info(u1)->depth == 10);
    CHECK(info(u1)->nodes == 1'250'000);
    CHECK(info(u1)->elapsed == 900ms);
    CHECK(info(u1)->score == Score{Score::Kind::Eval, -34});
    CHECK(info(u1)->principalVariation == std::vector<Coord>{{7, 7}, {8, 8}});
    REQUIRE(info(u1)->candidates.size() == 1);
    CHECK(info(u1)->candidates[0].rank == 1);
    CHECK(info(u1)->candidates[0].move == Coord{7, 7});
    CHECK(info(u1)->candidates[0].score == Score{Score::Kind::Eval, -34});
    CHECK_FALSE(info(u1)->candidates[0].visits);
    CHECK(info(u1)->candidates[0].principalVariation == std::vector<Coord>{{7, 7}, {8, 8}});

    auto u2 = parseGomocupLine(
        "MESSAGE depth 10-14 multipv 2 ev 12 n 1.25M n/ms 800 tm 900 pv E5 B2");
    REQUIRE(info(u2));
    CHECK(info(u2)->depth == 10);
    CHECK(info(u2)->nodes == 1'250'000);
    CHECK(info(u2)->elapsed == 900ms);
    CHECK_FALSE(info(u2)->score);
    CHECK(info(u2)->principalVariation.empty());
    REQUIRE(info(u2)->candidates.size() == 1);
    CHECK(info(u2)->candidates[0].rank == 2);
    CHECK(info(u2)->candidates[0].move == Coord{4, 4});
    CHECK(info(u2)->candidates[0].score == Score{Score::Kind::Eval, 12});
    CHECK(info(u2)->candidates[0].principalVariation == std::vector<Coord>{{4, 4}, {1, 1}});

    // Form 4: UCI-like realtime/MCTS carrying both candidate `v` and total `n`.
    auto r1 = parseGomocupLine(
        "MESSAGE multipv 1 ev 42 w 61.25 d 12.50 stdev 0.15 v 1.5K seldepth 14 "
        "n 2.4M tm 500 prior 0.30 pv H8 G8");
    REQUIRE(info(r1));
    CHECK(info(r1)->nodes == 2'400'000);
    CHECK(info(r1)->elapsed == 500ms);
    CHECK(info(r1)->score == Score{Score::Kind::Eval, 42});
    CHECK(info(r1)->principalVariation == std::vector<Coord>{{7, 7}, {6, 7}});
    REQUIRE(info(r1)->candidates.size() == 1);
    CHECK(info(r1)->candidates[0].rank == 1);
    CHECK(info(r1)->candidates[0].move == Coord{7, 7});
    CHECK(info(r1)->candidates[0].score == Score{Score::Kind::Eval, 42});
    CHECK(info(r1)->candidates[0].visits == 1500);
    REQUIRE(info(r1)->candidates[0].winrate);
    REQUIRE(info(r1)->candidates[0].drawrate);
    CHECK(*info(r1)->candidates[0].winrate == doctest::Approx(0.6125));
    CHECK(*info(r1)->candidates[0].drawrate == doctest::Approx(0.125));
    CHECK(info(r1)->candidates[0].principalVariation == std::vector<Coord>{{7, 7}, {6, 7}});

    auto r2 = parseGomocupLine(
        "MESSAGE multipv 2 ev -10 w 40.00 d 20.00 stdev 0.20 v 500 seldepth 10 "
        "n 2.4M prior 0.10 pv 1,1 2,2");
    REQUIRE(info(r2));
    CHECK(info(r2)->nodes == 2'400'000);
    CHECK_FALSE(info(r2)->score);
    CHECK(info(r2)->principalVariation.empty());
    REQUIRE(info(r2)->candidates.size() == 1);
    CHECK(info(r2)->candidates[0].rank == 2);
    CHECK(info(r2)->candidates[0].move == Coord{1, 1});
    CHECK(info(r2)->candidates[0].score == Score{Score::Kind::Eval, -10});
    CHECK(info(r2)->candidates[0].visits == 500);
    REQUIRE(info(r2)->candidates[0].winrate);
    REQUIRE(info(r2)->candidates[0].drawrate);
    CHECK(*info(r2)->candidates[0].winrate == doctest::Approx(0.4));
    CHECK(*info(r2)->candidates[0].drawrate == doctest::Approx(0.2));
    CHECK(info(r2)->candidates[0].principalVariation == std::vector<Coord>{{1, 1}, {2, 2}});

    // Out-of-range winrate / drawrate values are dropped rather than stored outside 0..1.
    auto badRate1 = parseGomocupLine("MESSAGE multipv 1 ev 0 w 105.00 d -1.00 pv H8");
    REQUIRE(info(badRate1));
    REQUIRE(info(badRate1)->candidates.size() == 1);
    CHECK_FALSE(info(badRate1)->candidates[0].winrate);
    CHECK_FALSE(info(badRate1)->candidates[0].drawrate);

    auto badRate2 = parseGomocupLine(
        "MESSAGE (1) 0 (W 120.00, D -2.00, S 0.10) | V 10 | SD 5 | H8");
    REQUIRE(info(badRate2));
    REQUIRE(info(badRate2)->candidates.size() == 1);
    CHECK(info(badRate2)->candidates[0].visits == 10);
    CHECK_FALSE(info(badRate2)->candidates[0].winrate);
    CHECK_FALSE(info(badRate2)->candidates[0].drawrate);
}

TEST_CASE("gomo7_forms: unranked lines keep candidates empty and unrecognised stay raw") {
    for (const char* line : {
             "MESSAGE depth 2-3 ev 0 n 73 n/ms 73 tm 1 pv E5 B2",
             "MESSAGE Bestline H7 J9",
             "MESSAGE Speed 12K | Visit 100 | Time 3ms",
             "MESSAGE Speed 333K | Depth 5-6 | Eval -67 | Node 373 | Time 1ms",
             "MESSAGE Depth 4-5 | Eval 0 | Time 1ms | E5 B2"}) {
        INFO(line);
        auto e = parseGomocupLine(line);
        REQUIRE(info(e));
        CHECK(info(e)->candidates.empty());
    }

    for (const char* line : {
             "MESSAGE (0) 34 | 5-6 | E5",
             "MESSAGE (x) 34 | 5-6 | E5",
             "MESSAGE (1) bad | nope",
             "MESSAGE multipv 0 ev 7 pv 1,1",
             "MESSAGE multipv x ev 7 pv 1,1",
             "MESSAGE multipv 1 unknown foo",
             "MESSAGE Visits 100 Winrate 50%",
             "MESSAGE OptiTime 1773ms | MaxTime 1970ms"}) {
        INFO(line);
        CHECK(std::holds_alternative<MessageEvent>(parseGomocupLine(line)));
    }

    for (const char* line : {"OK", "(1) 34 | 5-6 | E5"}) {
        INFO(line);
        auto e = parseGomocupLine(line);
        REQUIRE(std::holds_alternative<UnparsedEvent>(e));
        CHECK(std::get<UnparsedEvent>(e).line == line);
    }
}

TEST_CASE("gomo7_fixture: real Rapfi nbest capture keeps ranks 1..3 and summary rules") {
    const std::string path = std::string{PYGOMO_FIXTURE_DIR} + "/rapfi-nbest-normal.txt";
    std::ifstream     in(path);
    REQUIRE_MESSAGE(in.good(), path);

    std::string line;
    int         rank1           = 0;
    int         rank2           = 0;
    int         rank3           = 0;
    int         unrankedInfos   = 0;
    int         moves           = 0;
    bool        sawDepth17Rank1 = false;
    bool        sawDepth17Rank2 = false;

    while (std::getline(in, line)) {
        if (!line.empty() && line[0] == '#') continue;
        auto e = parseGomocupLine(line);
        if (auto* u = std::get_if<UnparsedEvent>(&e)) CHECK(u->line == line);
        if (auto* m = std::get_if<MessageEvent>(&e)) CHECK("MESSAGE " + m->text == line);
        if (std::holds_alternative<BestMoveEvent>(e)) ++moves;

        if (line.rfind("MESSAGE (", 0) == 0) {
            REQUIRE_MESSAGE(info(e), line);
            REQUIRE(info(e)->candidates.size() == 1);
            const auto& c = info(e)->candidates[0];
            REQUIRE(c.rank);
            CHECK(*c.rank >= 1);
            CHECK(*c.rank <= 3);
            CHECK(c.score.has_value());
            CHECK(c.move.has_value());
            CHECK_FALSE(c.principalVariation.empty());
            CHECK(c.move == c.principalVariation.front());
            CHECK(info(e)->depth.has_value());
            CHECK_FALSE(info(e)->nodes);

            if (*c.rank == 1) {
                ++rank1;
                CHECK(info(e)->score == c.score);
                CHECK(info(e)->principalVariation == c.principalVariation);
                if (info(e)->depth == 17 && c.score == Score{Score::Kind::Eval, 128} &&
                    c.move == Coord{8, 6}) {
                    sawDepth17Rank1 = true;
                }
            } else {
                if (*c.rank == 2) {
                    ++rank2;
                    if (info(e)->depth == 17 && c.score == Score{Score::Kind::Eval, 130} &&
                        c.move == Coord{7, 6}) {
                        sawDepth17Rank2 = true;
                    }
                } else {
                    ++rank3;
                }
                CHECK_FALSE(info(e)->score);
                CHECK(info(e)->principalVariation.empty());
            }
        } else if (info(e)) {
            ++unrankedInfos;
            CHECK(info(e)->candidates.empty());
        }
    }

    CHECK(rank1 >= 1);
    CHECK(rank2 >= 1);
    CHECK(rank3 >= 1);
    CHECK(unrankedInfos >= 1);
    CHECK(moves == 1);
    // Depth 17 has rank 2 (eval 130) above rank 1 (eval 128); engine rank must not be re-sorted.
    CHECK(sawDepth17Rank1);
    CHECK(sawDepth17Rank2);
}
