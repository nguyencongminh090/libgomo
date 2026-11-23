#include <doctest.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "pygomo/katagomo/katagomo_encode.hpp"
#include "pygomo/katagomo/katagomo_engine.hpp"
#include "pygomo/parse/gomocup_parser.hpp"

using namespace pygomo;

namespace {

/// Records what the adapter writes; the test plays the engine by calling say().
class FakeTransport final : public Transport {
  public:
    std::expected<void, TransportError> start(TransportListener& l) override {
        listener = &l;
        return {};
    }
    std::expected<void, TransportError> writeLine(std::string_view line) override {
        std::lock_guard lock(m);
        if (failWrites) return std::unexpected(TransportError::Closed);
        written.emplace_back(line);
        if (onWrite) onWrite(std::string{line});
        if (failWrites) return std::unexpected(TransportError::Closed);
        return {};
    }
    void                     close() override {}
    void                     say(std::string_view l) { listener->onLine(l); }
    void                     closeNow() { listener->onClosed(CloseReason::Eof); }
    std::vector<std::string> lines() {
        std::lock_guard lock(m);
        return written;
    }

    std::mutex                              m;
    std::vector<std::string>                written;
    TransportListener*                      listener = nullptr;
    std::function<void(const std::string&)> onWrite;
    // set by onWrite to fail that write and every later one
    bool failWrites = false;
};

struct Collect final : EventSink {
    void onEvent(const SearchEvent& e) override {
        std::lock_guard l(m);
        events.push_back(e);
    }
    std::vector<SearchEvent> get() {
        std::lock_guard l(m);
        return events;
    }
    std::mutex               m;
    std::vector<SearchEvent> events;
};

Position threeMoves() {
    auto p = Position::create(15, 15);
    REQUIRE(p);
    REQUIRE(p->play({7, 7}));
    REQUIRE(p->play({8, 7}));
    REQUIRE(p->play({7, 8}));
    return *p;
}

struct Rig {
    FakeTransport  t;
    KataGomoEngine e{t};
    Rig() { REQUIRE(e.open()); }
};

using Lines = std::vector<std::string>;

}  // namespace

TEST_CASE("kata_sequence: START, rule, BOARD with plain x,y, DONE") {
    Rig     r;
    Collect sink;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, sink));
    CHECK(r.t.lines() == Lines{"START 15", "FREESTYLE", "BOARD", "7,7", "8,7", "7,8", "DONE"});
    CHECK(r.e.startSearch(threeMoves(), Limits{}, sink).error() == EngineError::Busy);

    Rig  e;
    auto empty = Position::create(15, 15);
    REQUIRE(empty);
    REQUIRE(e.e.startSearch(*empty, Limits{}, sink));
    CHECK(e.t.lines() == Lines{"START 15", "FREESTYLE", "BOARD", "DONE"});
}

TEST_CASE("kata_rules: rule commands; swap openings are Unsupported and write nothing") {
    Rig     r;
    Collect sink;
    CHECK(r.e.setRules(Rules{BoardRule::Renju, OpeningRule::FreeOpen, 200}));
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, sink));
    CHECK(r.t.lines()[1] == "RENJU");
    CHECK(r.t.lines()[2] == "maxmoves 200");

    Rig u;
    for (auto opening : {OpeningRule::Swap1, OpeningRule::Swap2}) {
        auto bad = u.e.setRules(Rules{BoardRule::Standard, opening, std::nullopt});
        REQUIRE_FALSE(bad);
        CHECK(bad.error() == EngineError::Unsupported);
    }
    CHECK(u.t.lines().empty());
}

TEST_CASE("kata_limits: turn time and nodes are sent; depth and match time are Unsupported") {
    Rig     r;
    Collect sink;
    Limits  lim;
    lim.turnTime = std::chrono::milliseconds{5000};
    lim.maxNodes = 1000;
    REQUIRE(r.e.startSearch(threeMoves(), lim, sink));
    CHECK(r.t.lines() == Lines{"START 15", "FREESTYLE", "INFO timeout_turn 5000",
                               "kata-set-param maxVisits 1000", "BOARD", "7,7", "8,7", "7,8",
                               "DONE"});

    Rig    u;
    Limits depth;
    depth.maxDepth = 5;
    Limits match;
    match.matchTime = std::chrono::milliseconds{60000};
    CHECK(u.e.startSearch(threeMoves(), depth, sink).error() == EngineError::Unsupported);
    CHECK(u.e.startSearch(threeMoves(), match, sink).error() == EngineError::Unsupported);
    CHECK(u.t.lines().empty());
}

TEST_CASE("kata_events: OK dropped, MESSAGE unparsed, x,y best move, ERROR terminal") {
    Rig     r;
    Collect sink;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, sink));
    r.t.say("OK");
    r.t.say("MESSAGE Visits 10 Winrate 50%");
    r.t.say("6,6");
    r.t.say("MESSAGE late");  // after the terminal event: dropped
    auto ev = sink.get();
    REQUIRE(ev.size() == 2);
    CHECK(std::holds_alternative<MessageEvent>(ev[0]));
    CHECK(std::get<BestMoveEvent>(ev[1]).coord == Coord{6, 6});

    Collect s2;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, s2));
    r.t.say("ERROR Move Outside Board");
    REQUIRE(s2.get().size() == 1);
    CHECK(std::holds_alternative<ErrorEvent>(s2.get()[0]));
    Collect s3;
    CHECK(r.e.startSearch(threeMoves(), Limits{}, s3));  // search ended, not Busy
}

TEST_CASE("port3_kata: a close during startSearch gives one outcome, not event plus error") {
    Rig     r;
    Collect sink;
    r.t.onWrite = [&](const std::string& l) {
        if (l != "BOARD") return;
        r.t.failWrites = true;
        r.t.closeNow();  // the reader sees EOF while startSearch is still writing
    };
    CHECK(r.e.startSearch(threeMoves(), Limits{}, sink));
    REQUIRE(sink.get().size() == 1);
    CHECK(std::holds_alternative<ErrorEvent>(sink.get()[0]));
}

TEST_CASE("port3_kata: a failed write while the search is live is refused with no event") {
    Rig     r;
    Collect sink;
    r.t.onWrite = [&](const std::string& l) { r.t.failWrites = l == "BOARD"; };
    CHECK(r.e.startSearch(threeMoves(), Limits{}, sink).error() == EngineError::TransportFailed);
    CHECK(sink.get().empty());
    r.t.onWrite    = nullptr;
    r.t.failWrites = false;
    CHECK(r.e.startSearch(threeMoves(), Limits{}, sink));  // the refused search left no sink
}

namespace {

// Turns "VC <n>" into a CustomEvent and leaves every other line to the default parser.
SearchEvent vcParser(std::string_view line) {
    if (line.starts_with("VC ")) {
        return CustomEvent{"vc",
                           {{"depth", std::int64_t{std::stoll(std::string{line.substr(3)})}}}};
    }
    return UnparsedEvent{std::string{line}};
}

}  // namespace

TEST_CASE("ext02_kata: a CustomEvent from the parser does not end the search") {
    FakeTransport  t;
    KataGomoEngine e{t, parse::chainParsers(vcParser, parse::parseGomocupLine)};
    REQUIRE(e.open());
    Collect sink;
    REQUIRE(e.startSearch(threeMoves(), Limits{}, sink));
    t.say("VC 7");
    CHECK(e.startSearch(threeMoves(), Limits{}, sink).error() == EngineError::Busy);
    t.say("6,6");
    auto ev = sink.get();
    REQUIRE(ev.size() == 2);
    CHECK(std::get<CustomEvent>(ev[0]).tag == "vc");
    CHECK(std::get<BestMoveEvent>(ev[1]).coord == Coord{6, 6});
}

TEST_CASE("ext04_kata: a custom parser sees the line first, except OK") {
    Lines seen;
    auto  parser = [&](std::string_view l) -> SearchEvent {
        seen.emplace_back(l);
        return MessageEvent{"mine"};
    };

    FakeTransport  t;
    KataGomoEngine e{t, parser};
    REQUIRE(e.open());
    Collect sink;
    REQUIRE(e.startSearch(threeMoves(), Limits{}, sink));
    t.say("OK");
    t.say("6,6");  // the custom parser answers, so the default never reads this move
    REQUIRE(sink.get().size() == 1);
    CHECK(std::get<MessageEvent>(sink.get()[0]).text == "mine");
    CHECK(seen == Lines{"6,6"});
}

TEST_CASE("ext04_kata: Unparsed falls back via chainParsers; no parser keeps old behaviour") {
    FakeTransport  t;
    KataGomoEngine e{t, parse::chainParsers(vcParser, parse::parseGomocupLine)};
    REQUIRE(e.open());
    Collect sink;
    REQUIRE(e.startSearch(threeMoves(), Limits{}, sink));
    t.say("OK");
    t.say("VC 1");
    t.say("MESSAGE Visits 10");
    t.say("6,6");
    auto ev = sink.get();
    REQUIRE(ev.size() == 3);
    CHECK(std::holds_alternative<CustomEvent>(ev[0]));
    CHECK(std::holds_alternative<MessageEvent>(ev[1]));
    CHECK(std::get<BestMoveEvent>(ev[2]).coord == Coord{6, 6});

    Rig     r;  // default constructor argument
    Collect s2;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, s2));
    r.t.say("VC 1");
    r.t.say("6,6");
    REQUIRE(s2.get().size() == 2);
    CHECK(std::holds_alternative<UnparsedEvent>(s2.get()[0]));
    CHECK(std::get<BestMoveEvent>(s2.get()[1]).coord == Coord{6, 6});
}

TEST_CASE("ext05_katagomo: rules with extensions are unsupported and write nothing") {
    Rig         r;
    const Rules rules{.extensions = {{"custom-rule", true}}};
    CHECK_FALSE(r.e.supports(rules));
    const auto result = r.e.setRules(rules);
    REQUIRE_FALSE(result);
    CHECK(result.error() == EngineError::Unsupported);
    CHECK(r.t.lines().empty());
}

TEST_CASE("ext08_kata_default: search writes the same gom script byte for byte") {
    Rig        empty;
    const auto position = Position::create(15, 15);
    REQUIRE(position);
    Collect sink;
    REQUIRE(empty.e.startSearch(*position, {}, sink));
    CHECK(empty.t.lines() == Lines{"START 15", "FREESTYLE", "BOARD", "DONE"});

    Rig game;
    REQUIRE(game.e.setRules(Rules{.board = BoardRule::Standard, .maxMoves = 100}));
    const Limits limits{.turnTime = std::chrono::milliseconds{2505}, .maxNodes = 4000};
    REQUIRE(game.e.startSearch(threeMoves(), limits, sink));
    CHECK(game.t.lines() == Lines{"START 15", "STANDARD", "maxmoves 100", "INFO timeout_turn 2505",
                                  "kata-set-param maxVisits 4000", "BOARD", "7,7", "8,7", "7,8",
                                  "DONE"});
}

TEST_CASE(
    "ext08_kata_builder: the default builder emits gom lines and refuses unsupported requests") {
    const auto position = Position::create(15, 15);
    REQUIRE(position);
    const auto encoded = encode::defaultKataGomoRequest(SearchRequest{{}, *position, {}});
    REQUIRE(encoded);
    CHECK(*encoded == Lines{"START 15", "FREESTYLE", "BOARD", "DONE"});

    const auto refused = [](const SearchRequest& request) {
        const auto result = encode::defaultKataGomoRequest(request);
        return !result && result.error() == EngineError::Unsupported;
    };
    CHECK(refused(SearchRequest{Rules{.opening = OpeningRule::Swap2}, *position, {}}));
    CHECK(refused(SearchRequest{Rules{.extensions = {{"custom-rule", true}}}, *position, {}}));
    CHECK(refused(SearchRequest{{}, *position, Limits{.matchTime = std::chrono::milliseconds{0}}}));
    CHECK(refused(SearchRequest{{}, *position, Limits{.maxDepth = 8}}));

    const auto rectangle = Position::create(20, 10);
    REQUIRE(rectangle);
    CHECK(refused(SearchRequest{{}, *rectangle, {}}));

    const auto torus = Position::create(15, 15, BoardLayout{.torus = true});
    REQUIRE(torus);
    CHECK(refused(SearchRequest{{}, *torus, {}}));
    Rig        r;
    Collect    sink;
    const auto rejected = r.e.startSearch(*torus, {}, sink);
    REQUIRE_FALSE(rejected);
    CHECK(rejected.error() == EngineError::Unsupported);
    CHECK(r.t.lines().empty());
}

TEST_CASE("ext08_kata_custom: an encoder replaces the entire script") {
    FakeTransport  t;
    KataGomoEngine e{t, {}, [](const SearchRequest& request) -> EncodedRequest {
                         if (!request.rules.extensions.contains("custom-rule") ||
                             !request.position.layout().torus || !request.limits.maxDepth) {
                             return std::unexpected(EngineError::Unsupported);
                         }
                         return Lines{"SIZE " + std::to_string(request.position.width()) + " " +
                                          std::to_string(request.position.height()),
                                      "RULE custom", "TORUS",
                                      "SEARCH " + std::to_string(*request.limits.maxDepth)};
                     }};
    REQUIRE(e.open());
    const Rules rules{.opening = OpeningRule::Swap2, .extensions = {{"custom-rule", true}}};
    CHECK(e.supports(rules));
    CHECK_FALSE(e.supports(Rules{.maxMoves = 0}));
    REQUIRE(e.setRules(rules));

    const auto position = Position::create(20, 10, BoardLayout{.torus = true});
    REQUIRE(position);
    Collect sink;
    REQUIRE(e.startSearch(*position, Limits{.maxDepth = 8}, sink));
    CHECK(t.lines() == Lines{"SIZE 20 10", "RULE custom", "TORUS", "SEARCH 8"});
    t.say("7,7");
    CHECK(sink.get() == std::vector<SearchEvent>{BestMoveEvent{Coord{7, 7}}});
}

TEST_CASE("ext08_kata_refuse: an encoder error or empty script leaves the engine idle") {
    FakeTransport  t;
    KataGomoEngine e{t, {}, [](const SearchRequest& request) -> EncodedRequest {
                         if (request.limits.maxDepth) {
                             return std::unexpected(EngineError::TransportFailed);
                         }
                         if (request.limits.matchTime) return Lines{};
                         return Lines{"SEARCH"};
                     }};
    REQUIRE(e.open());
    const auto position = Position::create(15, 15);
    REQUIRE(position);
    Collect      sink;
    const Limits depth{.maxDepth = 1};
    const auto   failed = e.startSearch(*position, depth, sink);
    REQUIRE_FALSE(failed);
    CHECK(failed.error() == EngineError::TransportFailed);
    const auto empty =
        e.startSearch(*position, Limits{.matchTime = std::chrono::milliseconds{0}}, sink);
    REQUIRE_FALSE(empty);
    CHECK(empty.error() == EngineError::Unsupported);
    CHECK(t.lines().empty());
    CHECK(sink.get().empty());

    REQUIRE(e.startSearch(*position, {}, sink));
    CHECK(t.lines() == Lines{"SEARCH"});
    const auto busy = e.startSearch(*position, depth, sink);
    REQUIRE_FALSE(busy);
    CHECK(busy.error() == EngineError::Busy);
    t.say("7,7");
    t.closeNow();
    const auto closed = e.startSearch(*position, depth, sink);
    REQUIRE_FALSE(closed);
    CHECK(closed.error() == EngineError::NotReady);
}

TEST_CASE("ext09_kata: a 15x10 position is Unsupported and nothing is written") {
    Rig        r;
    Collect    sink;
    const auto p = Position::create(15, 10);
    REQUIRE(p);
    CHECK(r.e.startSearch(*p, {}, sink).error() == EngineError::Unsupported);
    CHECK(r.t.lines().empty());
}
