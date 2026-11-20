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
    InfoEvent e{.depth = 14,
                .nodes = 4'000'000'000LL,
                .score = Score{Score::Kind::MateIn, 5},
                .elapsed = 1200ms,
                .principalVariation = {{7, 7}, {8, 8}}};
    CHECK(*e.depth == 14);
    CHECK(*e.nodes == 4'000'000'000LL);
    CHECK(e.score->kind == Score::Kind::MateIn);
    CHECK(e.principalVariation.size() == 2);
    CHECK(e == InfoEvent{e});
}

TEST_CASE("Unparsed keeps the original line verbatim") {
    const std::string raw = "  weird   engine\toutput ";
    SearchEvent e = UnparsedEvent{raw};
    CHECK(std::get<UnparsedEvent>(e).line == raw);
}

TEST_CASE("Events compare by value, across alternatives") {
    CHECK(SearchEvent{BestMoveEvent{{1, 2}}} == SearchEvent{BestMoveEvent{{1, 2}}});
    CHECK(SearchEvent{BestMoveEvent{{1, 2}}} != SearchEvent{BestMoveEvent{{2, 1}}});
    CHECK(SearchEvent{SwapEvent{}} != SearchEvent{PassEvent{}});
}
