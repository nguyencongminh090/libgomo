// Live tests against a real Rapfi. Set PYGOMO_RAPFI to the pbrain-rapfi executable; without it
// every test prints SKIPPED (ctest treats that as skipped, not passed).
#include <doctest.h>

#include <signal.h>
#include <unistd.h>

#include <condition_variable>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "pygomo/engine/blocking_search.hpp"
#include "pygomo/engine/deadline_engine.hpp"
#include "pygomo/gomocup/gomocup_engine.hpp"
#include "pygomo/parse/gomocup_parser.hpp"
#include "pygomo/process/process_transport.hpp"

using namespace pygomo;
using namespace std::chrono_literals;

namespace {

// Path of the engine binary from the environment, or null when the live tests are off.
const char* rapfiPath() { return std::getenv("PYGOMO_RAPFI"); }

// Skips the current test case when no engine binary is configured.
#define REQUIRE_RAPFI()                                                                            \
    if (!rapfiPath()) {                                                                            \
        std::cout << "SKIPPED: PYGOMO_RAPFI not set\n";                                            \
        return;                                                                                    \
    }

// Rapfi V3 binary from the environment, or null when the rectangular-board live test is off.
const char* rapfiV3Path() { return std::getenv("PYGOMO_RAPFI_V3"); }

// Black (7,7), White (8,7), Black (7,8): a small non-empty 15x15 start.
Position threeMoves() {
    auto p = Position::create(15, 15);
    REQUIRE(p);
    REQUIRE(p->play({7, 7}));
    REQUIRE(p->play({8, 7}));
    REQUIRE(p->play({7, 8}));
    return *p;
}

// Sink that records every event and wakes wait() once a terminal event arrives.
struct Terminal final : EventSink {
    void onEvent(const SearchEvent& e) override {
        std::lock_guard lock(m);
        events.push_back(e);
        if (!std::holds_alternative<InfoEvent>(e) && !std::holds_alternative<MessageEvent>(e) &&
            !std::holds_alternative<UnparsedEvent>(e)) {
            done = true;
            cv.notify_all();
        }
    }
    bool wait(std::chrono::milliseconds t) {
        std::unique_lock lock(m);
        return cv.wait_for(lock, t, [&] { return done; });
    }
    std::mutex               m;
    std::condition_variable  cv;
    std::vector<SearchEvent> events;
    bool                     done = false;
};

// Collects parsed events from raw transport lines and wakes wait() on BestMove or close.
struct RawListener final : TransportListener {
    void onLine(std::string_view line) override {
        auto            ev = parse::parseGomocupLine(line);
        std::lock_guard lock(m);
        if (auto* info = std::get_if<InfoEvent>(&ev)) {
            infos.push_back(*info);
        } else if (std::holds_alternative<BestMoveEvent>(ev)) {
            gotMove = true;
            done    = true;
            cv.notify_all();
        }
    }
    void onClosed(CloseReason) override {
        std::lock_guard lock(m);
        done = true;
        cv.notify_all();
    }
    bool wait(std::chrono::milliseconds timeout) {
        std::unique_lock lock(m);
        return cv.wait_for(lock, timeout, [&] { return done; });
    }
    std::mutex              m;
    std::condition_variable cv;
    std::vector<InfoEvent>  infos;
    bool                    gotMove = false;
    bool                    done    = false;
};

// Launch options for the engine named by PYGOMO_RAPFI.
ProcessOptions rapfi() {
    ProcessOptions o;
    o.program = rapfiPath();
    return o;
}

/// The pid of this process's only child (the engine), read from /proc; -1 if there is none.
pid_t childPid() {
    pid_t found = -1;
    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        const std::string name = entry.path().filename().string();
        if (name.empty() || name.find_first_not_of("0123456789") != std::string::npos) continue;
        std::ifstream stat(entry.path() / "stat");
        std::string   line;
        if (!std::getline(stat, line)) continue;
        const auto close = line.rfind(')');  // the name in parentheses may contain spaces
        if (close == std::string::npos) continue;
        char state = 0;
        long ppid  = 0;
        if (std::sscanf(line.c_str() + close + 1, " %c %ld", &state, &ppid) == 2 &&
            ppid == getpid()) {
            if (found != -1) return -1;  // more than one child: do not guess
            found = static_cast<pid_t>(std::stol(name));
        }
    }
    return found;
}

}  // namespace

TEST_CASE("gomo2_live: a real search returns a legal move and info") {
    REQUIRE_RAPFI();
    ProcessTransport t(rapfi());
    GomocupEngine    engine(t);
    REQUIRE(engine.open());
    Limits lim;
    lim.turnTime = 1000ms;
    auto pos     = threeMoves();
    auto out     = search(engine, pos, lim);
    REQUIRE(out);
    REQUIRE(std::holds_alternative<BestMoveEvent>(out->terminal));
    auto c = std::get<BestMoveEvent>(out->terminal).coord;
    CHECK(pos.contains(c));
    CHECK_FALSE(pos.occupied(c));
    CHECK(out->info.size() >= 1);
    t.close();
}

TEST_CASE("gomo2_stop: stop() during a long search still yields a move") {
    REQUIRE_RAPFI();
    ProcessTransport t(rapfi());
    GomocupEngine    engine(t);
    REQUIRE(engine.open());
    Limits lim;
    lim.turnTime = 60'000ms;
    Terminal sink;
    auto     pos = threeMoves();
    REQUIRE(engine.startSearch(pos, lim, sink));
    std::this_thread::sleep_for(500ms);
    engine.stop();
    bool ok = sink.wait(4s);
    if (!ok) {  // the source documents a race between stop and the finish callback
        engine.stop();
        ok = sink.wait(4s);
    }
    REQUIRE(ok);
    std::lock_guard lock(sink.m);
    REQUIRE(std::holds_alternative<BestMoveEvent>(sink.events.back()));
    CHECK(pos.contains(std::get<BestMoveEvent>(sink.events.back()).coord));
    t.close();
}

TEST_CASE("gomo2_close: closing mid-search gives one error event and no process") {
    REQUIRE_RAPFI();
    ProcessTransport t(rapfi());
    GomocupEngine    engine(t);
    REQUIRE(engine.open());
    Limits lim;
    lim.turnTime = 60'000ms;
    Terminal sink;
    REQUIRE(engine.startSearch(threeMoves(), lim, sink));
    std::this_thread::sleep_for(300ms);
    t.close();
    REQUIRE(sink.wait(3s));
    {
        std::lock_guard lock(sink.m);
        int             errors = 0;
        for (auto& e : sink.events) errors += std::holds_alternative<ErrorEvent>(e);
        CHECK(errors == 1);
    }
    CHECK(t.exitStatus().has_value());  // reaped: no zombie, no live child
}

TEST_CASE("gomo2_deadline: a frozen engine times out, its late move is dropped, "
          "then it is usable again") {
    REQUIRE_RAPFI();
    ProcessTransport t(rapfi());
    GomocupEngine    engine(t);
    REQUIRE(engine.open());
    DeadlineEngine guarded(engine, DeadlineEngine::Options{300ms});
    const pid_t    child = childPid();
    REQUIRE(child > 0);

    Limits lim;
    lim.turnTime = 200ms;
    Terminal first;
    auto     pos = threeMoves();
    REQUIRE(guarded.startSearch(pos, lim, first));
    REQUIRE(kill(child, SIGSTOP) == 0);  // frozen: reads nothing, answers nothing
    const bool timedOut = first.wait(5s);
    {
        std::lock_guard lock(first.m);
        REQUIRE(timedOut);
        REQUIRE(first.events.size() == 1);
        REQUIRE(std::holds_alternative<ErrorEvent>(first.events[0]));
        CHECK(std::get<ErrorEvent>(first.events[0]).message == "engine did not answer");
    }
    Terminal second;
    CHECK(guarded.startSearch(pos, lim, second).error() == EngineError::Busy);

    REQUIRE(kill(child, SIGCONT) == 0);  // it now reads YXSTOP and sends the move it owes
    bool free = false;
    for (int i = 0; i < 100 && !free; ++i) {
        free = guarded.startSearch(pos, Limits{}, second).has_value();
        if (!free) std::this_thread::sleep_for(50ms);
    }
    REQUIRE(free);
    guarded.stop();
    REQUIRE(second.wait(5s));
    {
        std::lock_guard lock(first.m);
        CHECK(first.events.size() == 1);  // the late move never reached the first sink
    }
    std::lock_guard lock(second.m);
    REQUIRE(std::holds_alternative<BestMoveEvent>(second.events.back()));
    CHECK(pos.contains(std::get<BestMoveEvent>(second.events.back()).coord));
    CHECK_FALSE(pos.occupied(std::get<BestMoveEvent>(second.events.back()).coord));
    t.close();
}

TEST_CASE("gomo7_live: real Rapfi MultiPV search yields a rank-2 candidate") {
    REQUIRE_RAPFI();
    RawListener      listener;
    ProcessTransport t(rapfi());
    REQUIRE(t.start(listener));
    // YXSHOWINFO switches Rapfi from brief to normal mode so ranked lines are printed.
    for (const char* cmd : {"START 15", "YXSHOWINFO", "INFO timeout_turn 1000", "YXBOARD", "7,7,1",
                            "8,8,2", "DONE", "YXNBEST 3"}) {
        REQUIRE(t.writeLine(cmd));
    }
    const bool finished = listener.wait(10s);
    t.close();
    REQUIRE(finished);
    CHECK(listener.gotMove);
    bool hasRank2 = false;
    for (const auto& info : listener.infos) {
        for (const auto& cand : info.candidates) {
            if (cand.rank == 2) hasRank2 = true;
        }
    }
    REQUIRE(hasRank2);
}

TEST_CASE("ext09_live: Rapfi V3 plays on a 15x10 board after RECTSTART") {
    if (!rapfiV3Path()) {
        std::cout << "SKIPPED: PYGOMO_RAPFI_V3 not set\n";
        return;
    }
    ProcessOptions o;
    o.program = rapfiV3Path();
    // Rapfi reads config.toml and its weights from the working directory, next to the binary.
    o.workingDirectory = std::filesystem::path(o.program).parent_path().string();
    ProcessTransport t(o);
    GomocupEngine    engine(t);
    REQUIRE(engine.open());
    DeadlineEngine guarded(engine, DeadlineEngine::Options{5000ms});

    auto pos = Position::create(15, 10);
    REQUIRE(pos);
    REQUIRE(pos->play({7, 4}));
    REQUIRE(pos->play({8, 4}));
    Limits lim;
    lim.turnTime = 1000ms;
    Terminal sink;
    REQUIRE(guarded.startSearch(*pos, lim, sink));
    REQUIRE(sink.wait(10s));
    t.close();
    std::lock_guard lock(sink.m);
    REQUIRE(std::holds_alternative<BestMoveEvent>(sink.events.back()));
    const Coord c = std::get<BestMoveEvent>(sink.events.back()).coord;
    CHECK(pos->contains(c));
    CHECK_FALSE(pos->occupied(c));
}
