#include <doctest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "pygomo/engine/blocking_search.hpp"
#include "pygomo/engine/deadline_engine.hpp"
#include "pygomo/gomocup/gomocup_engine.hpp"

using namespace pygomo;

namespace {

/// Records what the adapter writes; the test plays the engine by calling say()/closeNow().
class FakeTransport final : public Transport {
public:
    std::expected<void, TransportError> start(TransportListener& l) override {
        listener = &l;
        return {};
    }
    std::expected<void, TransportError> writeLine(std::string_view line) override {
        std::lock_guard lock(m);
        written.emplace_back(line);
        if (onWrite) onWrite(std::string{line});
        return {};
    }
    void close() override {}
    void say(std::string_view l) { listener->onLine(l); }
    void closeNow() { listener->onClosed(CloseReason::Eof); }

    std::vector<std::string> lines() { std::lock_guard lock(m); return written; }

    std::mutex m;
    std::vector<std::string> written;
    std::function<void(const std::string&)> onWrite;
    TransportListener* listener = nullptr;
};

struct Collect final : EventSink {
    void onEvent(const SearchEvent& e) override { std::lock_guard l(m); events.push_back(e); }
    std::vector<SearchEvent> get() { std::lock_guard l(m); return events; }
    std::mutex m;
    std::vector<SearchEvent> events;
};

Position threeMoves(int w = 15, int h = 15) {
    auto p = Position::create(w, h);
    REQUIRE(p);
    REQUIRE(p->play({7, 7}));
    REQUIRE(p->play({8, 7}));
    REQUIRE(p->play({7, 8}));
    return *p;
}

struct Rig {
    FakeTransport t;
    GomocupEngine e{t};
    Rig() { REQUIRE(e.open()); }
};

/// The engine behind a DeadlineEngine, which owns the deadline since DEAD-03.
struct DeadlineRig {
    FakeTransport t;
    GomocupEngine g{t};
    DeadlineEngine e;
    explicit DeadlineRig(std::chrono::milliseconds grace)
        : e(g, DeadlineEngine::Options{grace}) {
        REQUIRE(g.open());
    }
};

using Lines = std::vector<std::string>;

}  // namespace

TEST_CASE("gomo_sequence: START, INFO, BOARD with owner flags, DONE") {
    Rig r;
    Collect sink;
    Limits lim;
    lim.turnTime = std::chrono::milliseconds{5000};
    lim.maxDepth = 12;
    REQUIRE(r.e.startSearch(threeMoves(), lim, sink));
    // Three moves played, so White is to move: White's stone (8,7) is flag 1.
    CHECK(r.t.lines() ==
          Lines{"START 15", "INFO rule 0", "INFO timeout_turn 5000", "INFO max_depth 12", "BOARD",
                "7,7,2", "8,7,1", "7,8,2", "DONE"});
}

TEST_CASE("gomo_sequence: an empty board uses BEGIN; second search while running is Busy") {
    Rig r;
    Collect sink;
    auto empty = Position::create(15, 15);
    REQUIRE(empty);
    REQUIRE(r.e.startSearch(*empty, Limits{}, sink));
    CHECK(r.t.lines().back() == "BEGIN");
    CHECK(r.e.startSearch(*empty, Limits{}, sink).error() == EngineError::Busy);
}

TEST_CASE("gomo_rules: mapping, unsupported combinations write nothing") {
    Rig r;
    Collect sink;
    CHECK(r.e.setRules(Rules{BoardRule::Renju, OpeningRule::FreeOpen, 200}));
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, sink));
    auto l = r.t.lines();
    CHECK(l[1] == "INFO rule 4");
    CHECK(l[2] == "INFO max_moves 200");

    Rig u;
    CHECK_FALSE(u.e.supports(Rules{BoardRule::Standard, OpeningRule::Swap1, std::nullopt}));
    auto bad = u.e.setRules(Rules{BoardRule::Standard, OpeningRule::Swap1, std::nullopt});
    REQUIRE_FALSE(bad);
    CHECK(bad.error() == EngineError::Unsupported);
    CHECK(u.t.lines().empty());

    Rig s;
    Collect sink2;
    CHECK(s.e.setRules(Rules{BoardRule::Freestyle, OpeningRule::Swap2, std::nullopt}));
    REQUIRE(s.e.startSearch(threeMoves(), Limits{}, sink2));
    CHECK(s.t.lines()[1] == "INFO rule 6");
}

TEST_CASE("gomo_events: info then best move; swap; illegal move") {
    Rig r;
    Collect sink;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, sink));
    r.t.say("OK");
    r.t.say("MESSAGE depth 5-8 ev 20 n 100K tm 50 pv 6,6");
    r.t.say("6,6");
    r.t.say("MESSAGE depth 9");  // after the terminal event: dropped
    auto ev = sink.get();
    REQUIRE(ev.size() == 3);
    CHECK(std::holds_alternative<UnparsedEvent>(ev[0]));
    CHECK(std::get<InfoEvent>(ev[1]).depth == 5);
    CHECK(std::get<BestMoveEvent>(ev[2]).coord == Coord{6, 6});

    Collect s2;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, s2));
    r.t.say("SWAP");
    CHECK(std::holds_alternative<SwapEvent>(s2.get().at(0)));

    for (const char* bad : {"7,7", "15,3", "3,15"}) {
        Collect s3;
        REQUIRE(r.e.startSearch(threeMoves(), Limits{}, s3));
        r.t.say(bad);
        REQUIRE(s3.get().size() == 1);
        CHECK(std::holds_alternative<ErrorEvent>(s3.get()[0]));
    }
}

TEST_CASE("gomo_stop: YXSTOP is sent and the following move is delivered") {
    Rig r;
    Collect sink;
    r.e.stop();  // no search: nothing written
    CHECK(r.t.lines().empty());
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, sink));
    r.e.stop();
    CHECK(r.t.lines().back() == "YXSTOP");
    r.t.say("5,5");
    REQUIRE(sink.get().size() == 1);
    CHECK(std::get<BestMoveEvent>(sink.get()[0]).coord == Coord{5, 5});
}

TEST_CASE("gomo_closed: one error event, no hang, later search is NotReady") {
    Rig r;
    Collect sink;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, sink));
    r.t.closeNow();
    REQUIRE(sink.get().size() == 1);
    CHECK(std::holds_alternative<ErrorEvent>(sink.get()[0]));
    CHECK(r.e.startSearch(threeMoves(), Limits{}, sink).error() == EngineError::NotReady);

    Rig b;
    std::thread closer;
    b.t.onWrite = [&](const std::string& l) {
        if (l == "DONE") closer = std::thread([&] { b.t.closeNow(); });
    };
    auto out = search(b.e, threeMoves(), Limits{});
    closer.join();
    REQUIRE(out);
    CHECK(std::holds_alternative<ErrorEvent>(out->terminal));
}

TEST_CASE("gomo_rect: a non-square board is Unsupported and writes nothing") {
    Rig r;
    Collect sink;
    CHECK(r.e.startSearch(threeMoves(15, 12), Limits{}, sink).error() == EngineError::Unsupported);
    CHECK(r.t.lines().empty());
}

TEST_CASE("gomo_warning: an ERROR line during a search does not end it") {
    Rig r;
    Collect sink;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, sink));
    r.t.say("ERROR Unknown rule id: 9. Rule is reset to freestyle...");
    r.t.say("6,6");
    auto ev = sink.get();
    REQUIRE(ev.size() == 2);
    CHECK(std::get<MessageEvent>(ev[0]).text ==
          "ERROR Unknown rule id: 9. Rule is reset to freestyle...");
    CHECK(std::get<BestMoveEvent>(ev[1]).coord == Coord{6, 6});

    // Still terminal: an illegal move, and a closed connection.
    Collect s2;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, s2));
    r.t.say("ERROR something");
    r.t.say("7,7");
    REQUIRE(s2.get().size() == 2);
    CHECK(std::holds_alternative<ErrorEvent>(s2.get()[1]));

    Collect s3;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, s3));
    r.t.say("ERROR something");
    r.t.closeNow();
    REQUIRE(s3.get().size() == 2);
    CHECK(std::holds_alternative<ErrorEvent>(s3.get()[1]));
}

namespace {

using namespace std::chrono_literals;

Limits turn(std::chrono::milliseconds ms) {
    Limits l;
    l.turnTime = ms;
    return l;
}

bool waitFor(Collect& s, std::size_t n) {
    for (int i = 0; i < 400; ++i) {
        if (s.get().size() >= n) return true;
        std::this_thread::sleep_for(5ms);
    }
    return false;
}

}  // namespace

TEST_CASE(
    "gomo_deadline: no move after turnTime+grace -> YXSTOP, "
    "then ErrorEvent, engine reusable") {
    DeadlineRig r{30ms};
    Collect sink;
    REQUIRE(r.e.startSearch(threeMoves(), turn(20ms), sink));
    r.t.say("ERROR something went wrong");  // a warning does not end the search
    REQUIRE(waitFor(sink, 2));
    auto ev = sink.get();
    REQUIRE(ev.size() == 2);
    CHECK(std::holds_alternative<MessageEvent>(ev[0]));
    CHECK(std::get<ErrorEvent>(ev[1]).message == "engine did not answer");
    CHECK(r.t.lines().back() == "YXSTOP");

    // The engine still owes its answer: no new search yet (GOMO-06, Busy since DEAD-03).
    Collect s2;
    CHECK(r.e.startSearch(threeMoves(), Limits{}, s2).error() == EngineError::Busy);
    r.t.say("6,6");  // the late move is dropped, then the engine is usable again
    CHECK(sink.get().size() == 2);
    CHECK(r.e.startSearch(threeMoves(), Limits{}, s2));
    CHECK(s2.get().empty());
}

TEST_CASE(
    "gomo_deadline: an engine that owes an answer ignores ERROR lines, "
    "accepts a swap, refuses after close") {
    DeadlineRig r{20ms};
    Collect a;
    REQUIRE(r.e.startSearch(threeMoves(), turn(10ms), a));
    REQUIRE(waitFor(a, 1));
    r.t.say("ERROR still busy");
    Collect b;
    CHECK(r.e.startSearch(threeMoves(), Limits{}, b).error() == EngineError::Busy);
    r.t.say("SWAP");
    CHECK(r.e.startSearch(threeMoves(), Limits{}, b));
    CHECK(b.get().empty());
    CHECK(a.get().size() == 1);

    DeadlineRig c{20ms};
    Collect d;
    REQUIRE(c.e.startSearch(threeMoves(), turn(10ms), d));
    REQUIRE(waitFor(d, 1));
    c.t.closeNow();
    Collect f;
    CHECK(c.e.startSearch(threeMoves(), Limits{}, f).error() == EngineError::NotReady);
}

TEST_CASE("gomo_deadline: a move in time or after YXSTOP ends the search normally") {
    DeadlineRig r{200ms};
    Collect a;
    REQUIRE(r.e.startSearch(threeMoves(), turn(10ms), a));
    r.t.say("6,6");
    std::this_thread::sleep_for(400ms);
    REQUIRE(a.get().size() == 1);
    CHECK(std::holds_alternative<BestMoveEvent>(a.get()[0]));

    Collect b;  // engine answers after YXSTOP, inside the second grace
    REQUIRE(r.e.startSearch(threeMoves(), turn(10ms), b));
    for (int i = 0; i < 400 && r.t.lines().back() != "YXSTOP"; ++i) {
        std::this_thread::sleep_for(2ms);
    }
    REQUIRE(r.t.lines().back() == "YXSTOP");
    r.t.say("6,6");
    std::this_thread::sleep_for(400ms);
    REQUIRE(b.get().size() == 1);
    CHECK(std::holds_alternative<BestMoveEvent>(b.get()[0]));
}

TEST_CASE(
    "gomo_deadline: no deadline without turnTime or with turnTime 0, "
    "a closed engine gets no timeout event") {
    DeadlineRig r{10ms};
    Collect a;
    REQUIRE(r.e.startSearch(threeMoves(), Limits{}, a));
    std::this_thread::sleep_for(150ms);
    CHECK(a.get().empty());
    for (const auto& l : r.t.lines()) CHECK(l != "YXSTOP");
    r.t.say("6,6");

    Collect z;  // turnTime 0 means no limit, not "expire at once"
    REQUIRE(r.e.startSearch(threeMoves(), turn(0ms), z));
    std::this_thread::sleep_for(150ms);
    CHECK(z.get().empty());

    r.t.closeNow();
    REQUIRE(a.get().size() == 1);
    CHECK(std::holds_alternative<BestMoveEvent>(a.get()[0]));
    REQUIRE(z.get().size() == 1);
    std::this_thread::sleep_for(100ms);
    CHECK(z.get().size() == 1);
}

TEST_CASE(
    "gomo_deadline: the sink may call startSearch from the timeout callback "
    "(refused while owed)") {
    DeadlineRig r{10ms};
    struct Again final : EventSink {
        DeadlineRig* rig{};
        Collect* next{};
        std::atomic<bool> called{false};
        std::atomic<bool> refused{false};
        void onEvent(const SearchEvent&) override {
            refused =
                rig->e.startSearch(threeMoves(), Limits{}, *next).error() == EngineError::Busy;
            called = true;
        }
    } again;
    Collect next;
    again.rig = &r;
    again.next = &next;
    REQUIRE(r.e.startSearch(threeMoves(), turn(10ms), again));
    for (int i = 0; i < 400 && !again.called; ++i) std::this_thread::sleep_for(5ms);
    CHECK(again.called);
    CHECK(again.refused);
}
