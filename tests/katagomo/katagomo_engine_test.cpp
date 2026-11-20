#include <doctest.h>

#include <mutex>
#include <string>
#include <vector>

#include "pygomo/katagomo/katagomo_engine.hpp"

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
        written.emplace_back(line);
        return {};
    }
    void close() override {}
    void say(std::string_view l) { listener->onLine(l); }
    std::vector<std::string> lines() { std::lock_guard lock(m); return written; }

    std::mutex m;
    std::vector<std::string> written;
    TransportListener* listener = nullptr;
};

struct Collect final : EventSink {
    void onEvent(const SearchEvent& e) override { std::lock_guard l(m); events.push_back(e); }
    std::vector<SearchEvent> get() { std::lock_guard l(m); return events; }
    std::mutex m;
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
    FakeTransport t;
    KataGomoEngine e{t};
    Rig() { REQUIRE(e.open()); }
};

using Lines = std::vector<std::string>;

}  // namespace

TEST_CASE("kata_sequence: START, rule, BOARD with plain x,y, DONE") {
    Rig r;
    Collect sink;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, sink));
    CHECK(r.t.lines() == Lines{"START 15", "FREESTYLE", "BOARD", "7,7", "8,7", "7,8", "DONE"});
    CHECK(r.e.startSearch(threeMoves(), Limits{}, sink).error() == EngineError::Busy);

    Rig e;
    auto empty = Position::create(15, 15);
    REQUIRE(empty);
    REQUIRE(e.e.startSearch(*empty, Limits{}, sink));
    CHECK(e.t.lines() == Lines{"START 15", "FREESTYLE", "BOARD", "DONE"});
}

TEST_CASE("kata_rules: rule commands; swap openings are Unsupported and write nothing") {
    Rig r;
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
    Rig r;
    Collect sink;
    Limits lim;
    lim.turnTime = std::chrono::milliseconds{5000};
    lim.maxNodes = 1000;
    REQUIRE(r.e.startSearch(threeMoves(), lim, sink));
    CHECK(r.t.lines() ==
          Lines{"START 15", "FREESTYLE", "INFO timeout_turn 5000", "kata-set-param maxVisits 1000",
                "BOARD", "7,7", "8,7", "7,8", "DONE"});

    Rig u;
    Limits depth;
    depth.maxDepth = 5;
    Limits match;
    match.matchTime = std::chrono::milliseconds{60000};
    CHECK(u.e.startSearch(threeMoves(), depth, sink).error() == EngineError::Unsupported);
    CHECK(u.e.startSearch(threeMoves(), match, sink).error() == EngineError::Unsupported);
    CHECK(u.t.lines().empty());
}

TEST_CASE("kata_events: OK dropped, MESSAGE unparsed, x,y best move, ERROR terminal") {
    Rig r;
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
