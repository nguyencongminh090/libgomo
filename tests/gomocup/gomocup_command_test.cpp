// EXT-10: CommandPort on GomocupEngine and through DeadlineEngine, against a fake Rapfi.
#include <doctest.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../support/command_fakes.hpp"
#include "pygomo/engine/deadline_engine.hpp"
#include "pygomo/gomocup/gomocup_engine.hpp"

using namespace pygomo;
using namespace pygomo::testing;
using namespace std::chrono_literals;

namespace {

Position threeMoves() {
    auto p = Position::create(15, 15);
    REQUIRE(p);
    REQUIRE(p->play({7, 7}));
    REQUIRE(p->play({8, 7}));
    REQUIRE(p->play({7, 8}));
    return *p;
}

Command vcThread() {
    return Command{.name = "INFO", .args = {std::string{"VCTHREAD"}, std::int64_t{2}}};
}

Command analyze() {
    return Command{.name       = "YXANALZ",
                   .block      = {Coord{7, 7}, Coord{8, 8}},
                   .terminator = "DONE",
                   .completion = Command::Search{}};
}

struct Rig {
    Log           log;
    FakeTransport t{log};
    GomocupEngine e{t};
    Rig() { REQUIRE(e.open()); }
};

struct DeadlineRig {
    Log            log;
    FakeTransport  t{log};
    GomocupEngine  g{t};
    DeadlineEngine e;
    explicit DeadlineRig(std::chrono::milliseconds grace) : e(g, DeadlineEngine::Options{grace}) {
        REQUIRE(g.open());
    }
};

Limits turn(std::chrono::milliseconds t) {
    Limits l;
    l.turnTime = t;
    return l;
}

}  // namespace

TEST_CASE("ext10_gomocup: a None command is written and nothing is awaited") {
    Rig     r;
    LogSink sink{r.log, "cmd"};
    REQUIRE(r.e.execute(vcThread(), sink));
    CHECK(r.t.lines() == Lines{"INFO VCTHREAD 2"});
    r.t.say("MESSAGE VCN Mode enabled: VC2");  // V2 reply: not awaited, dropped
    CHECK(sink.get().empty());
    LogSink search{r.log, "search"};
    CHECK(r.e.startSearch(threeMoves(), Limits{}, search));
}

TEST_CASE("ext10_gomocup: a YXANALZ Search command sends its block and ends at the move") {
    Rig     r;
    LogSink sink{r.log, "cmd"};
    REQUIRE(r.e.execute(analyze(), sink));
    CHECK(r.t.lines() == Lines{"YXANALZ", "7,7", "8,8", "DONE"});
    LogSink other{r.log, "other"};
    CHECK(r.e.startSearch(threeMoves(), Limits{}, other).error() == EngineError::Busy);
    CHECK(r.e.execute(vcThread(), other).error() == EngineError::Busy);

    r.t.say("MESSAGE analysing");
    r.t.say("MESSAGE DEPTH 5 EV 30");
    r.t.say("7,7");  // no position: the move is not checked against an old search
    auto ev = sink.get();
    REQUIRE(ev.size() == 3);
    CHECK(std::holds_alternative<MessageEvent>(ev[0]));
    CHECK(ev[2] == SearchEvent{BestMoveEvent{Coord{7, 7}}});
    CHECK(r.e.startSearch(threeMoves(), Limits{}, other));
}

TEST_CASE("ext10_until: ends at the first matching event, which still reaches the sink") {
    Rig     r;
    LogSink sink{r.log, "cmd"};
    Command c{.name = "YXSHOWINFO", .completion = Command::Until{[](const SearchEvent& e) {
                                        const auto* m = std::get_if<MessageEvent>(&e);
                                        return m && m->text.starts_with("INFO END");
                                    }}};
    REQUIRE(r.e.execute(c, sink));
    CHECK(r.t.lines() == Lines{"YXSHOWINFO"});
    LogSink other{r.log, "other"};
    r.t.say("MESSAGE INFO MAX_THREAD_NUM 1");
    r.t.say("8,8");  // a move does not end an Until command
    CHECK(r.e.startSearch(threeMoves(), Limits{}, other).error() == EngineError::Busy);
    r.t.say("MESSAGE INFO END");
    auto ev = sink.get();
    REQUIRE(ev.size() == 3);
    CHECK(ev[1] == SearchEvent{BestMoveEvent{Coord{8, 8}}});
    CHECK(std::get<MessageEvent>(ev[2]).text == "INFO END");
    r.t.say("MESSAGE late");  // after the end: dropped
    CHECK(sink.get().size() == 3);
    CHECK(r.e.startSearch(threeMoves(), Limits{}, other));

    Command noPredicate{.name = "X", .completion = Command::Until{}};
    CHECK(r.e.execute(noPredicate, sink).error() == EngineError::Unsupported);
    CHECK(r.e.execute(Command{}, sink).error() == EngineError::Unsupported);
}

TEST_CASE("ext10_busy: during a search, execute without force is Busy and writes nothing") {
    Rig     r;
    LogSink search{r.log, "search"};
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, search));
    const auto before = r.t.lines();
    LogSink    cmd{r.log, "cmd"};
    CHECK(r.e.execute(vcThread(), cmd).error() == EngineError::Busy);
    CHECK(r.e.execute(analyze(), cmd).error() == EngineError::Busy);
    CHECK(r.t.lines() == before);
}

TEST_CASE("ext10_force: YXSTOP, the old move reaches the old sink, then the command is written") {
    Rig     r;
    LogSink search{r.log, "search"};
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, search));
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(analyze(), cmd, {.force = true}));
    CHECK(r.t.lines().back() == "YXSTOP");  // the command itself is not written yet

    LogSink second{r.log, "second"};
    CHECK(r.e.execute(vcThread(), second, {.force = true}).error() == EngineError::Busy);
    CHECK(r.e.startSearch(threeMoves(), Limits{}, second).error() == EngineError::Busy);

    r.t.say("9,9");
    CHECK(after(r.log.get(), "w YXSTOP") ==
          Lines{"w YXSTOP", "search best", "w YXANALZ", "w 7,7", "w 8,8", "w DONE"});
    r.t.say("8,8");  // the command's own search ends at its move
    CHECK(cmd.get() == std::vector<SearchEvent>{BestMoveEvent{Coord{8, 8}}});
    CHECK(second.get().empty());
}

TEST_CASE("ext10_force: the old sink's terminal callback cannot start work before the command") {
    Rig     r;
    LogSink search{r.log, "search"};
    LogSink cmd{r.log, "cmd"};
    LogSink other{r.log, "other"};
    bool    called = false;
    search.hook    = [&](const SearchEvent&) {
        called = true;
        CHECK(r.e.startSearch(threeMoves(), Limits{}, other).error() == EngineError::Busy);
        CHECK(r.e.execute(vcThread(), other, {.force = true}).error() == EngineError::Busy);
    };
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, search));
    REQUIRE(r.e.execute(vcThread(), cmd, {.force = true}));
    r.t.say("9,9");
    CHECK(called);
    CHECK(r.t.lines().back() == "INFO VCTHREAD 2");
    CHECK(cmd.get().empty());  // a None command that was written gets no event
    CHECK(r.e.startSearch(threeMoves(), Limits{}, other));  // None: idle once written
}

TEST_CASE("ext10_force: a move that arrives while YXSTOP is written still precedes the command") {
    Rig     r;
    LogSink search{r.log, "search"};
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, search));
    // The engine answers YXSTOP at once, on the writing thread, before execute regains its lock.
    r.t.onWrite = [&](const std::string& line) {
        if (line == "YXSTOP") r.t.say("9,9");
    };
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(analyze(), cmd, {.force = true}));
    CHECK(after(r.log.get(), "w YXSTOP") ==
          Lines{"w YXSTOP", "search best", "w YXANALZ", "w 7,7", "w 8,8", "w DONE"});
}

TEST_CASE("ext10_close: a queued command is dropped with one ErrorEvent after the old one") {
    Rig     r;
    LogSink search{r.log, "search"};
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, search));
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(analyze(), cmd, {.force = true}));
    r.t.closeNow();
    CHECK(after(r.log.get(), "w YXSTOP") == Lines{"w YXSTOP", "search error", "cmd error"});
    CHECK(std::get<ErrorEvent>(cmd.get()[0]).message ==
          "engine connection closed; command not sent");
    CHECK(r.e.execute(vcThread(), cmd).error() == EngineError::NotReady);

    Rig     n;  // a queued None command also learns it was not sent
    LogSink s2{n.log, "search"};
    REQUIRE(n.e.startSearch(threeMoves(), Limits{}, s2));
    LogSink none{n.log, "none"};
    REQUIRE(n.e.execute(vcThread(), none, {.force = true}));
    n.t.closeNow();
    CHECK(none.get().size() == 1);

    Rig     running;  // a running Search command ends with ErrorEvent on close
    LogSink s3{running.log, "cmd"};
    REQUIRE(running.e.execute(analyze(), s3));
    running.t.closeNow();
    REQUIRE(s3.get().size() == 1);
    CHECK(std::holds_alternative<ErrorEvent>(s3.get()[0]));
}

TEST_CASE("ext10_force: a queued command whose write fails gets ErrorEvent; the engine is free") {
    Rig     r;
    LogSink search{r.log, "search"};
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, search));
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(analyze(), cmd, {.force = true}));
    r.t.failOn = "YXANALZ";
    r.t.say("9,9");
    REQUIRE(cmd.get().size() == 1);
    CHECK(std::get<ErrorEvent>(cmd.get()[0]).message == "command could not be sent");
    r.t.failOn.reset();
    CHECK(r.e.startSearch(threeMoves(), Limits{}, search));
}

TEST_CASE("ext10_deadline: forwards execute with no deadline on the command") {
    DeadlineRig r{20ms};
    LogSink     cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(vcThread(), cmd));
    REQUIRE(r.e.execute(analyze(), cmd));
    CHECK(r.t.lines() == Lines{"INFO VCTHREAD 2", "YXANALZ", "7,7", "8,8", "DONE"});
    LogSink other{r.log, "other"};
    CHECK(r.e.startSearch(threeMoves(), Limits{}, other).error() == EngineError::Busy);
    std::this_thread::sleep_for(60ms);  // longer than any grace: a command has no deadline
    CHECK(cmd.get().empty());
    r.t.say("7,7");
    CHECK(cmd.get() == std::vector<SearchEvent>{BestMoveEvent{Coord{7, 7}}});
}

namespace {

/// An EnginePort that is not a CommandPort.
struct PlainEngine final : EnginePort {
    bool                             supports(const Rules&) const override { return true; }
    std::expected<void, EngineError> setRules(const Rules&) override { return {}; }
    std::expected<void, EngineError> startSearch(const Position&, const Limits&,
                                                 EventSink&) override {
        return {};
    }
    void                             stop() override {}
    std::expected<void, EngineError> sendRaw(std::string_view) override { return {}; }
};

}  // namespace

TEST_CASE("ext10_deadline: a wrapped engine that is no CommandPort gives Unsupported") {
    PlainEngine    plain;
    DeadlineEngine e{plain};
    Log            log;
    LogSink        cmd{log, "cmd"};
    CHECK(e.execute(vcThread(), cmd).error() == EngineError::Unsupported);
}

TEST_CASE("ext10_deadline: force waits for the search's move, then passes the command on") {
    DeadlineRig r{2000ms};
    LogSink     search{r.log, "search"};
    REQUIRE(r.e.startSearch(threeMoves(), turn(5000ms), search));
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(analyze(), cmd, {.force = true}));
    CHECK(r.t.lines().back() == "YXSTOP");
    LogSink other{r.log, "other"};
    CHECK(r.e.execute(vcThread(), other, {.force = true}).error() == EngineError::Busy);
    r.t.say("9,9");
    CHECK(after(r.log.get(), "w YXSTOP") ==
          Lines{"w YXSTOP", "search best", "w YXANALZ", "w 7,7", "w 8,8", "w DONE"});
    r.t.say("8,8");
    CHECK(cmd.get() == std::vector<SearchEvent>{BestMoveEvent{Coord{8, 8}}});
}

TEST_CASE("ext10_deadline: after a timeout the kept command fails and is never written") {
    DeadlineRig r{30ms};
    LogSink     search{r.log, "search"};
    REQUIRE(r.e.startSearch(threeMoves(), turn(10ms), search));
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(analyze(), cmd, {.force = true}));
    REQUIRE(waitFor(cmd, 1));  // the engine never answers YXSTOP
    CHECK(std::get<ErrorEvent>(search.get()[0]).message == "engine did not answer");
    CHECK(std::get<ErrorEvent>(cmd.get()[0]).message == "engine did not answer; command not sent");
    auto log = r.log.get();
    CHECK(after(log, "search error") == Lines{"search error", "cmd error"});

    // The engine may still be thinking: nothing is written until its late answer (AUD-03).
    LogSink later{r.log, "later"};
    CHECK(r.e.execute(vcThread(), later).error() == EngineError::Busy);
    CHECK(r.e.execute(vcThread(), later, {.force = true}).error() == EngineError::Busy);
    for (const auto& l : r.t.lines()) CHECK(l != "YXANALZ");
    r.t.say("9,9");  // the late move frees the engine
    REQUIRE(r.e.execute(vcThread(), later));
    CHECK(r.t.lines().back() == "INFO VCTHREAD 2");
    CHECK(cmd.get().size() == 1);
}

namespace {

/// Plays a move from a second thread when YXSTOP is written, and holds the old sink's
/// terminal callback open until execute has returned: the command must not be written while
/// the old sink is still handling the move.
struct SlowOldSink {
    std::mutex              m;
    std::condition_variable cv;
    bool                    entered  = false;
    bool                    returned = false;
    bool                    early    = false;  // a command line was written inside the callback
    std::thread             mover;

    void arm(FakeTransport& t, LogSink& old) {
        old.hook = [this, &t](const SearchEvent&) {
            std::unique_lock lock(m);
            entered = true;
            cv.notify_all();
            cv.wait(lock, [this] { return returned; });
            for (const auto& l : t.lines()) early = early || l == "YXANALZ";
        };
        t.onWrite = [this, &t](const std::string& line) {
            if (line != "YXSTOP") return;
            mover = std::thread([&t] { t.say("9,9"); });
            std::unique_lock lock(m);
            cv.wait(lock, [this] { return entered; });
        };
    }
    void executeReturned() {
        {
            std::lock_guard lock(m);
            returned = true;
        }
        cv.notify_all();
        mover.join();
    }
};

}  // namespace

TEST_CASE("ext10_force: a move read during the YXSTOP write: command waits for the old sink") {
    Rig         r;
    LogSink     search{r.log, "search"};
    SlowOldSink slow;
    slow.arm(r.t, search);
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, search));
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(analyze(), cmd, {.force = true}));
    slow.executeReturned();
    CHECK_FALSE(slow.early);
    CHECK(after(r.log.get(), "w YXSTOP") ==
          Lines{"w YXSTOP", "search best", "w YXANALZ", "w 7,7", "w 8,8", "w DONE"});
}

TEST_CASE("ext10_deadline: a move read during stop(): command waits for the old sink") {
    DeadlineRig r{2000ms};
    LogSink     search{r.log, "search"};
    SlowOldSink slow;
    slow.arm(r.t, search);
    REQUIRE(r.e.startSearch(threeMoves(), turn(5000ms), search));
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(analyze(), cmd, {.force = true}));
    slow.executeReturned();
    CHECK_FALSE(slow.early);
    CHECK(after(r.log.get(), "w YXSTOP") ==
          Lines{"w YXSTOP", "search best", "w YXANALZ", "w 7,7", "w 8,8", "w DONE"});
}

TEST_CASE("ext10_force: YXSTOP never lands inside another thread's search script") {
    Rig  r;
    Gate gate;
    r.t.onWrite = [&](const std::string& line) {
        if (line == "BOARD") gate.hold();
    };
    LogSink     search{r.log, "search"};
    std::thread starter([&] { CHECK(r.e.startSearch(threeMoves(), Limits{}, search)); });
    gate.waitHeld();  // the script stops in the middle of its BOARD block
    LogSink     cmd{r.log, "cmd"};
    std::thread forcer([&] { CHECK(r.e.execute(analyze(), cmd, {.force = true})); });
    std::this_thread::sleep_for(50ms);  // time for a YXSTOP to slip in if writes are not ordered
    gate.open();
    starter.join();
    forcer.join();
    const auto lines = r.t.lines();
    CHECK(indexOf(lines, "DONE") < indexOf(lines, "YXSTOP"));
    CHECK(lines.back() == "YXSTOP");
    r.t.say("9,9");
    CHECK(after(r.log.get(), "search best") ==
          Lines{"search best", "w YXANALZ", "w 7,7", "w 8,8", "w DONE"});
}

TEST_CASE("ext10_force: a failed search script leaves no forced command stuck behind it") {
    Rig  r;
    Gate gate;
    r.t.onWrite = [&](const std::string& line) {
        if (line == "BOARD") gate.hold();
    };
    r.t.failOn = "7,8,2";  // the script's last stone cannot be written
    LogSink                          search{r.log, "search"};
    std::expected<void, EngineError> started;
    std::thread starter([&] { started = r.e.startSearch(threeMoves(), Limits{}, search); });
    gate.waitHeld();
    LogSink                          cmd{r.log, "cmd"};
    std::expected<void, EngineError> forced;
    std::thread forcer([&] { forced = r.e.execute(analyze(), cmd, {.force = true}); });
    std::this_thread::sleep_for(50ms);
    gate.open();
    starter.join();
    forcer.join();
    REQUIRE_FALSE(started);
    CHECK(started.error() == EngineError::TransportFailed);
    CHECK(search.get().empty());
    // The forced call waited for the whole script, then ran on the now idle engine.
    REQUIRE(forced);
    CHECK(cmd.get().empty());
    const auto lines = r.t.lines();
    CHECK(Lines(lines.end() - 4, lines.end()) == Lines{"YXANALZ", "7,7", "8,8", "DONE"});
    r.t.failOn.reset();
    r.t.say("8,8");  // ends the command's search, if it runs
    LogSink next{r.log, "next"};
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, next));
    LogSink later{r.log, "later"};
    REQUIRE(r.e.execute(vcThread(), later, {.force = true}));  // not Busy forever
    r.t.say("9,9");
    CHECK(after(r.log.get(), "next best") == Lines{"next best", "w INFO VCTHREAD 2"});
}
