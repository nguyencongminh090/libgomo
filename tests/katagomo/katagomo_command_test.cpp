// EXT-10: CommandPort on KataGomoEngine, whose stop() cannot interrupt a search.
#include <doctest.h>

#include <cstdint>
#include <expected>
#include <string>
#include <thread>
#include <vector>

#include "../support/command_fakes.hpp"
#include "pygomo/katagomo/katagomo_engine.hpp"

using namespace pygomo;
using namespace pygomo::testing;

namespace {

Position twoMoves() {
    auto p = Position::create(15, 15);
    REQUIRE(p);
    REQUIRE(p->play({7, 7}));
    REQUIRE(p->play({8, 7}));
    return *p;
}

Command setParam() {
    return Command{.name = "kata-set-param", .args = {std::string{"maxVisits"}, std::int64_t{100}}};
}

Command boardOnly() {
    return Command{.name       = "YXBOARD",
                   .block      = {Coord{7, 7}, Coord{8, 8}},
                   .terminator = "DONE",
                   .completion = Command::Search{}};
}

struct Rig {
    Log            log;
    FakeTransport  t{log};
    KataGomoEngine e{t};
    Rig() { REQUIRE(e.open()); }
};

}  // namespace

TEST_CASE("ext10_kata: None and Search commands are written; a Search ends at its move") {
    Rig     r;
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(setParam(), cmd));
    REQUIRE(r.e.execute(boardOnly(), cmd));
    CHECK(r.t.lines() == Lines{"kata-set-param maxVisits 100", "YXBOARD", "7,7", "8,8", "DONE"});
    LogSink other{r.log, "other"};
    CHECK(r.e.startSearch(twoMoves(), Limits{}, other).error() == EngineError::Busy);
    r.t.say("MESSAGE thinking");
    r.t.say("7,7");
    auto ev = cmd.get();
    REQUIRE(ev.size() == 2);
    CHECK(ev[1] == SearchEvent{BestMoveEvent{Coord{7, 7}}});
    CHECK(r.e.startSearch(twoMoves(), Limits{}, other));
}

TEST_CASE("ext10_kata: force writes nothing until the search ends by itself") {
    Rig     r;
    LogSink search{r.log, "search"};
    REQUIRE(r.e.startSearch(twoMoves(), Limits{}, search));
    const auto before = r.t.lines();
    LogSink    cmd{r.log, "cmd"};
    CHECK(r.e.execute(setParam(), cmd).error() == EngineError::Busy);
    REQUIRE(r.e.execute(setParam(), cmd, {.force = true}));
    CHECK(r.t.lines() == before);  // no stop line: the engine cannot be interrupted
    CHECK(r.e.execute(setParam(), cmd, {.force = true}).error() == EngineError::Busy);
    r.t.say("9,9");
    CHECK(after(r.log.get(), "search best") ==
          Lines{"search best", "w kata-set-param maxVisits 100"});
    CHECK(cmd.get().empty());
    CHECK(r.e.startSearch(twoMoves(), Limits{}, search));
}

TEST_CASE("ext10_kata: close drops a queued command with one ErrorEvent") {
    Rig     r;
    LogSink search{r.log, "search"};
    REQUIRE(r.e.startSearch(twoMoves(), Limits{}, search));
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(boardOnly(), cmd, {.force = true}));
    r.t.closeNow();
    CHECK(after(r.log.get(), "search error") == Lines{"search error", "cmd error"});
    CHECK(cmd.get().size() == 1);
}

TEST_CASE("ext10_kata: a failed search script drops the command forced behind it") {
    Rig  r;
    Gate gate;
    r.t.onWrite = [&](const std::string& line) {
        if (line == "BOARD") gate.hold();
    };
    r.t.failOn = "DONE";  // the script's block cannot be closed
    LogSink                          search{r.log, "search"};
    std::expected<void, EngineError> started;
    std::thread starter([&] { started = r.e.startSearch(twoMoves(), Limits{}, search); });
    gate.waitHeld();
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(setParam(), cmd, {.force = true}));  // queued behind the search
    gate.open();
    starter.join();
    REQUIRE_FALSE(started);
    CHECK(started.error() == EngineError::TransportFailed);
    REQUIRE(cmd.get().size() == 1);  // the search it waited for has no terminal event left
    CHECK(std::holds_alternative<ErrorEvent>(cmd.get()[0]));

    r.t.failOn.reset();
    LogSink next{r.log, "next"};
    REQUIRE(r.e.startSearch(twoMoves(), Limits{}, next));
    LogSink later{r.log, "later"};
    REQUIRE(r.e.execute(setParam(), later, {.force = true}));  // not Busy forever
    r.t.say("9,9");
    // Only the new command follows the next search: the dropped one is never written.
    CHECK(after(r.log.get(), "next best") == Lines{"next best", "w kata-set-param maxVisits 100"});
    CHECK(cmd.get().size() == 1);
}
