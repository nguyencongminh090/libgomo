#include <doctest.h>

#include <string>
#include <type_traits>

#include "pygomo/core/search_event.hpp"

using namespace pygomo;
using namespace std::chrono_literals;

namespace {

// std::visit without a catch-all: adding an alternative to SearchEvent breaks this build,
// which is the point (every consumer must decide what to do with a new event).
std::string describe(const SearchEvent& e) {
    struct Visitor {
        std::string operator()(const InfoEvent&) const { return "info"; }
        std::string operator()(const BestMoveEvent&) const { return "bestmove"; }
        std::string operator()(const SwapEvent&) const { return "swap"; }
        std::string operator()(const PassEvent&) const { return "pass"; }
        std::string operator()(const ResignEvent&) const { return "resign"; }
        std::string operator()(const ErrorEvent&) const { return "error"; }
        std::string operator()(const MessageEvent&) const { return "message"; }
        std::string operator()(const UnparsedEvent&) const { return "unparsed"; }
    };
    return std::visit(Visitor{}, e);
}

}  // namespace

TEST_CASE("Every alternative is reachable through the variant") {
    CHECK(describe(InfoEvent{}) == "info");
    CHECK(describe(BestMoveEvent{{7, 7}}) == "bestmove");
    CHECK(describe(SwapEvent{}) == "swap");
    CHECK(describe(PassEvent{}) == "pass");
    CHECK(describe(ResignEvent{}) == "resign");
    CHECK(describe(ErrorEvent{"boom"}) == "error");
    CHECK(describe(MessageEvent{"hi"}) == "message");
    CHECK(describe(UnparsedEvent{"???"}) == "unparsed");
}

TEST_CASE("InfoEvent has no mandatory field") {
    InfoEvent e;
    CHECK_FALSE(e.depth.has_value());
    CHECK_FALSE(e.nodes.has_value());
    CHECK_FALSE(e.score.has_value());
    CHECK_FALSE(e.elapsed.has_value());
    CHECK(e.principalVariation.empty());
}

TEST_CASE("InfoEvent carries what the engine reported") {
    InfoEvent e{.depth              = 14,
                .nodes              = 4'000'000'000LL,
                .score              = Score{Score::Kind::MateIn, 5},
                .elapsed            = 1200ms,
                .principalVariation = {{7, 7}, {8, 8}}};
    CHECK(*e.depth == 14);
    CHECK(*e.nodes == 4'000'000'000LL);
    CHECK(e.score->kind == Score::Kind::MateIn);
    CHECK(e.principalVariation.size() == 2);
    CHECK(e == InfoEvent{e});
}

TEST_CASE("Unparsed keeps the original line verbatim") {
    const std::string raw = "  weird   engine\toutput ";
    SearchEvent       e   = UnparsedEvent{raw};
    CHECK(std::get<UnparsedEvent>(e).line == raw);
}

TEST_CASE("Events compare by value, across alternatives") {
    CHECK(SearchEvent{BestMoveEvent{{1, 2}}} == SearchEvent{BestMoveEvent{{1, 2}}});
    CHECK(SearchEvent{BestMoveEvent{{1, 2}}} != SearchEvent{BestMoveEvent{{2, 1}}});
    CHECK(SearchEvent{SwapEvent{}} != SearchEvent{PassEvent{}});
}

TEST_CASE("cand1_defaults: default InfoEvent and Candidate leave all fields empty") {
    InfoEvent info;
    CHECK(info.candidates.empty());

    Candidate c;
    CHECK_FALSE(c.rank.has_value());
    CHECK_FALSE(c.move.has_value());
    CHECK_FALSE(c.score.has_value());
    CHECK_FALSE(c.visits.has_value());
    CHECK_FALSE(c.winrate.has_value());
    CHECK_FALSE(c.drawrate.has_value());
    CHECK(c.principalVariation.empty());
}

TEST_CASE("cand1_equality: Candidate compares every field by value") {
    const Candidate base{
        .rank               = 1,
        .move               = Coord{7, 7},
        .score              = Score{Score::Kind::Eval, 25},
        .visits             = 120,
        .winrate            = 0.55,
        .drawrate           = 0.10,
        .principalVariation = {{7, 7}, {8, 8}},
    };
    CHECK(base == Candidate{base});

    auto diffRank = base;
    diffRank.rank = 2;
    CHECK(base != diffRank);

    auto diffMove = base;
    diffMove.move = Coord{6, 6};
    CHECK(base != diffMove);

    auto diffScore  = base;
    diffScore.score = Score{Score::Kind::Eval, 26};
    CHECK(base != diffScore);

    auto diffVisits   = base;
    diffVisits.visits = 121;
    CHECK(base != diffVisits);

    auto diffWinrate    = base;
    diffWinrate.winrate = 0.56;
    CHECK(base != diffWinrate);

    auto diffDrawrate     = base;
    diffDrawrate.drawrate = 0.11;
    CHECK(base != diffDrawrate);

    auto diffPv               = base;
    diffPv.principalVariation = {{7, 7}};
    CHECK(base != diffPv);
}

TEST_CASE("cand1_equality: InfoEvents differing only in candidates compare unequal") {
    InfoEvent a;
    a.depth              = 12;
    a.nodes              = 1000;
    a.score              = Score{Score::Kind::Eval, 7};
    a.elapsed            = 100ms;
    a.principalVariation = {{1, 1}};

    InfoEvent b = a;
    CHECK(a == b);

    b.candidates.push_back(Candidate{.rank = 1, .move = Coord{1, 1}});
    CHECK(a != b);

    InfoEvent c = b;
    CHECK(b == c);
    c.candidates[0].rank = 2;
    CHECK(b != c);
}

TEST_CASE("cand1_compat: existing InfoEvent forms still compile and compare equal") {
    InfoEvent empty{};
    CHECK(empty == InfoEvent{});
    CHECK(empty.candidates.empty());

    // Positional aggregate form without candidates relies on candidates being the last member.
    InfoEvent positional{10, 500, Score{Score::Kind::Eval, 12}, 50ms, {{7, 7}}};
    CHECK(positional.candidates.empty());
    CHECK(positional == InfoEvent{positional});

    InfoEvent designated{
        .depth              = 10,
        .nodes              = 500,
        .score              = Score{Score::Kind::Eval, 12},
        .elapsed            = 50ms,
        .principalVariation = {{7, 7}},
    };
    CHECK(designated == positional);
}

TEST_CASE("ext01_is_terminal: only Info, Message and Unparsed keep a search running") {
    CHECK_FALSE(isTerminal(InfoEvent{}));
    CHECK_FALSE(isTerminal(MessageEvent{"hello"}));
    CHECK_FALSE(isTerminal(UnparsedEvent{"???"}));
    CHECK(isTerminal(BestMoveEvent{{7, 7}}));
    CHECK(isTerminal(SwapEvent{}));
    CHECK(isTerminal(PassEvent{}));
    CHECK(isTerminal(ResignEvent{}));
    CHECK(isTerminal(ErrorEvent{"boom"}));
    static_assert(isTerminal(SearchEvent{SwapEvent{}}));
}
