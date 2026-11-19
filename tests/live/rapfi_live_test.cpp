// Live tests against a real Rapfi. Set PYGOMO_RAPFI to the pbrain-rapfi executable; without it
// every test prints SKIPPED (ctest treats that as skipped, not passed).
#include <doctest.h>

#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include "pygomo/engine/blocking_search.hpp"
#include "pygomo/gomocup/gomocup_engine.hpp"
#include "pygomo/process/process_transport.hpp"

using namespace pygomo;
using namespace std::chrono_literals;

namespace {

const char* rapfiPath() { return std::getenv("PYGOMO_RAPFI"); }

#define REQUIRE_RAPFI()                                  \
    if (!rapfiPath()) {                                  \
        std::cout << "SKIPPED: PYGOMO_RAPFI not set\n";  \
        return;                                          \
    }

Position threeMoves() {
    auto p = Position::create(15, 15);
    REQUIRE(p);
    REQUIRE(p->play({7, 7}));
    REQUIRE(p->play({8, 7}));
    REQUIRE(p->play({7, 8}));
    return *p;
}

struct Terminal final : EventSink {
    void onEvent(const SearchEvent& e) override {
        std::lock_guard lock(m);
        events.push_back(e);
        if (!std::holds_alternative<InfoEvent>(e) && !std::holds_alternative<UnparsedEvent>(e)) {
            done = true;
            cv.notify_all();
        }
    }
    bool wait(std::chrono::milliseconds t) {
        std::unique_lock lock(m);
        return cv.wait_for(lock, t, [&] { return done; });
    }
    std::mutex m;
    std::condition_variable cv;
    std::vector<SearchEvent> events;
    bool done = false;
};

ProcessOptions rapfi() {
    ProcessOptions o;
    o.program = rapfiPath();
    return o;
}

}  // namespace

TEST_CASE("gomo2_live: a real search returns a legal move and info") {
    REQUIRE_RAPFI();
    ProcessTransport t(rapfi());
    GomocupEngine engine(t);
    REQUIRE(engine.open());
    Limits lim;
    lim.turnTime = 1000ms;
    auto pos = threeMoves();
    auto out = search(engine, pos, lim);
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
    GomocupEngine engine(t);
    REQUIRE(engine.open());
    Limits lim;
    lim.turnTime = 60'000ms;
    Terminal sink;
    auto pos = threeMoves();
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
    GomocupEngine engine(t);
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
        int errors = 0;
        for (auto& e : sink.events) errors += std::holds_alternative<ErrorEvent>(e);
        CHECK(errors == 1);
    }
    CHECK(t.exitStatus().has_value());  // reaped: no zombie, no live child
}
