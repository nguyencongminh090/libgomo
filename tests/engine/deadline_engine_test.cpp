#include <doctest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <latch>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "pygomo/engine/deadline_engine.hpp"

using namespace pygomo;
using namespace std::chrono_literals;

namespace {

/// An engine the test plays: it stays busy until the test answers, like the real adapters.
class FakePort final : public EnginePort {
public:
    bool supports(const Rules&) const override { return true; }
    std::expected<void, EngineError> setRules(const Rules&) override { return {}; }
    std::expected<void, EngineError> startSearch(
        const Position&, const Limits&, EventSink& s) override {
        std::lock_guard l(m);
        if (failNext) return std::unexpected(EngineError::TransportFailed);
        if (sink) return std::unexpected(EngineError::Busy);
        sink = &s;
        ++starts;
        return {};
    }
    void stop() override { ++stops; }
    std::expected<void, EngineError> sendRaw(std::string_view) override { return {}; }

    /// The engine says something; a terminal event releases the sink first, as the adapters do.
    void say(const SearchEvent& e, bool terminal) {
        EventSink* s = nullptr;
        {
            std::lock_guard l(m);
            s = sink;
            if (terminal) sink = nullptr;
        }
        if (s) s->onEvent(e);
    }

    std::mutex m;
    EventSink* sink     = nullptr;
    bool       failNext = false;
    std::atomic<int> stops{0};
    int starts = 0;
};

struct Collect final : EventSink {
    void onEvent(const SearchEvent& e) override { std::lock_guard l(m); events.push_back(e); }
    std::vector<SearchEvent> get() { std::lock_guard l(m); return events; }
    std::mutex m;
    std::vector<SearchEvent> events;
};

Position pos() {
    auto p = Position::create(15, 15);
    REQUIRE(p);
    REQUIRE(p->play({7, 7}));
    return *p;
}

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

const SearchEvent kMove = BestMoveEvent{{8, 8}};

}  // namespace

TEST_CASE(
    "dead1_timeout: stop at turnTime+grace, ErrorEvent after another grace, "
    "Busy until the late answer") {
    FakePort port;
    DeadlineEngine eng{port, DeadlineEngine::Options{30ms}};
    Collect a;
    REQUIRE(eng.startSearch(pos(), turn(20ms), a));
    port.say(MessageEvent{"warming up"}, false);
    REQUIRE(waitFor(a, 2));
    auto ev = a.get();
    CHECK(std::holds_alternative<MessageEvent>(ev[0]));
    CHECK(std::get<ErrorEvent>(ev[1]).message == "engine did not answer");
    CHECK(port.stops >= 1);

    Collect b;
    CHECK(eng.startSearch(pos(), Limits{}, b).error() == EngineError::Busy);
    port.say(InfoEvent{}, false);  // late progress: dropped
    CHECK(eng.startSearch(pos(), Limits{}, b).error() == EngineError::Busy);
    port.say(kMove, true);  // the late answer: dropped, engine usable again
    CHECK(a.get().size() == 2);
    CHECK(eng.startSearch(pos(), Limits{}, b));
    port.say(kMove, true);
    REQUIRE(b.get().size() == 1);
    CHECK(std::holds_alternative<BestMoveEvent>(b.get()[0]));
}

TEST_CASE("dead1_intime: an answer before or after stop() ends the search normally") {
    FakePort port;
    DeadlineEngine eng{port, DeadlineEngine::Options{200ms}};
    Collect a;
    REQUIRE(eng.startSearch(pos(), turn(10ms), a));
    port.say(kMove, true);
    std::this_thread::sleep_for(400ms);
    REQUIRE(a.get().size() == 1);
    CHECK(port.stops == 0);

    Collect b;
    REQUIRE(eng.startSearch(pos(), turn(10ms), b));
    for (int i = 0; i < 400 && port.stops == 0; ++i) std::this_thread::sleep_for(2ms);
    REQUIRE(port.stops == 1);
    port.say(kMove, true);
    std::this_thread::sleep_for(400ms);
    REQUIRE(b.get().size() == 1);
    CHECK(std::holds_alternative<BestMoveEvent>(b.get()[0]));
}

TEST_CASE(
    "dead1_nodeadline: no deadline without turnTime or with 0; "
    "an error from the engine passes through") {
    FakePort port;
    DeadlineEngine eng{port, DeadlineEngine::Options{10ms}};
    Collect a;
    REQUIRE(eng.startSearch(pos(), Limits{}, a));
    std::this_thread::sleep_for(150ms);
    CHECK(a.get().empty());
    CHECK(port.stops == 0);
    port.say(kMove, true);

    Collect z;
    REQUIRE(eng.startSearch(pos(), turn(0ms), z));
    std::this_thread::sleep_for(150ms);
    CHECK(z.get().empty());
    port.say(ErrorEvent{"engine connection closed"}, true);
    REQUIRE(z.get().size() == 1);
    CHECK(std::get<ErrorEvent>(z.get()[0]).message == "engine connection closed");

    Collect busy;
    port.failNext = true;  // a start error leaves no state behind
    CHECK(eng.startSearch(pos(), turn(10ms), busy).error() == EngineError::TransportFailed);
    port.failNext = false;
    std::this_thread::sleep_for(100ms);
    CHECK(busy.get().empty());
    CHECK(eng.startSearch(pos(), Limits{}, busy));
}

TEST_CASE(
    "dead1_callback: the sink may call startSearch from the timeout callback "
    "and from a terminal event") {
    FakePort port;
    DeadlineEngine eng{port, DeadlineEngine::Options{10ms}};
    struct Again final : EventSink {
        DeadlineEngine* eng{};
        Collect* next{};
        std::atomic<int> calls{0};
        std::atomic<bool> busy{false};
        std::atomic<bool> started{false};
        void onEvent(const SearchEvent& e) override {
            ++calls;
            auto r = eng->startSearch(pos(), Limits{}, *next);
            if (std::holds_alternative<ErrorEvent>(e)) busy = !r && r.error() == EngineError::Busy;
            else started = r.has_value();
        }
    } again;
    Collect next;
    again.eng  = &eng;
    again.next = &next;

    REQUIRE(eng.startSearch(pos(), turn(10ms), again));
    for (int i = 0; i < 400 && again.calls == 0; ++i) std::this_thread::sleep_for(5ms);
    CHECK(again.busy);  // timed out: still owes its answer
    port.say(kMove, true);

    again.calls = 0;
    REQUIRE(eng.startSearch(pos(), Limits{}, again));
    port.say(kMove, true);  // terminal event: the sink starts the next search at once
    CHECK(again.started);
    CHECK(port.starts == 3);
}

TEST_CASE("dead1_serialization: the timeout waits for an active info callback to return") {
    struct HoldInfo final : EventSink {
        void onEvent(const SearchEvent& event) override {
            if (std::holds_alternative<InfoEvent>(event)) {
                infoStarted.set_value();
                releaseInfo.wait();
            }
            events.onEvent(event);
            if (std::holds_alternative<ErrorEvent>(event)) timeoutDelivered.set_value();
        }

        std::promise<void> infoStarted;
        std::latch         releaseInfo{1};
        std::promise<void> timeoutDelivered;
        Collect            events;
    } first;
    Collect next;
    FakePort port;
    DeadlineEngine eng{port, DeadlineEngine::Options{10ms}};
    auto infoStarted = first.infoStarted.get_future();
    auto timeout = first.timeoutDelivered.get_future();
    const auto position = pos();

    bool answerWhileBlocked = false;
    SUBCASE("late answer before timeout delivery") { answerWhileBlocked = true; }
    SUBCASE("late answer after timeout delivery") {}

    REQUIRE(eng.startSearch(position, turn(100ms), first));
    std::jthread adapter([&] { port.say(InfoEvent{}, false); });
    const bool infoEntered = infoStarted.wait_for(2s) == std::future_status::ready;
    const bool timedOutWhileBlocked = timeout.wait_for(250ms) == std::future_status::ready;
    const bool stopRequested = port.stops == 1;
    bool busyWhilePending   = false;
    bool lateAnswerReturned = false;
    std::jthread lateAnswer;
    if (answerWhileBlocked) {
        std::promise<void> returned;
        auto answer = returned.get_future();
        lateAnswer = std::jthread([&port, returned = std::move(returned)]() mutable {
            port.say(kMove, true);
            returned.set_value();
        });
        lateAnswerReturned = answer.wait_for(2s) == std::future_status::ready;
        auto retry = eng.startSearch(position, Limits{}, next);
        busyWhilePending = !retry && retry.error() == EngineError::Busy;
    }
    first.releaseInfo.count_down();
    adapter.join();
    if (lateAnswer.joinable()) lateAnswer.join();

    REQUIRE(infoEntered);
    CHECK(stopRequested);
    CHECK_FALSE(timedOutWhileBlocked);
    REQUIRE(timeout.wait_for(2s) == std::future_status::ready);
    auto events = first.events.get();
    REQUIRE(events.size() == 2);
    CHECK(std::holds_alternative<InfoEvent>(events[0]));
    REQUIRE(std::holds_alternative<ErrorEvent>(events[1]));
    CHECK(std::get<ErrorEvent>(events[1]).message == "engine did not answer");

    if (answerWhileBlocked) {
        CHECK(lateAnswerReturned);
        CHECK(busyWhilePending);
    } else {
        auto retry = eng.startSearch(position, Limits{}, next);
        REQUIRE_FALSE(retry);
        CHECK(retry.error() == EngineError::Busy);
        port.say(kMove, true);
    }
    CHECK(first.events.get().size() == 2);
    REQUIRE(eng.startSearch(position, Limits{}, next));
    port.say(kMove, true);
    events = next.get();
    REQUIRE(events.size() == 1);
    CHECK(std::holds_alternative<BestMoveEvent>(events[0]));
}

namespace {

/// Breaks the EnginePort contract on purpose: its first startSearch ends the search through the
/// sink and then refuses, as an adapter whose transport closes between installing the sink and
/// writing the request. The next `refuseAfter` calls refuse plainly; later calls start normally.
class EndsThenRefuses final : public EnginePort {
public:
    bool supports(const Rules&) const override { return true; }
    std::expected<void, EngineError> setRules(const Rules&) override { return {}; }
    std::expected<void, EngineError> startSearch(
        const Position&, const Limits&, EventSink& s) override {
        if (calls++ == 0) {
            s.onEvent(ErrorEvent{"engine connection closed"});
            return std::unexpected(EngineError::TransportFailed);
        }
        if (refuseAfter > 0) {
            --refuseAfter;
            return std::unexpected(EngineError::NotReady);
        }
        sink = &s;
        return {};
    }
    void stop() override {}
    std::expected<void, EngineError> sendRaw(std::string_view) override { return {}; }

    int        refuseAfter = 0;
    int        calls       = 0;
    EventSink* sink        = nullptr;
};

/// Starts the next search from the terminal callback of the first one.
struct Restart final : EventSink {
    Restart(DeadlineEngine& e, Limits l, Collect& n) : eng(e), limits(l), next(n) {}
    void onEvent(const SearchEvent&) override { restarted = eng.startSearch(pos(), limits, next); }

    DeadlineEngine& eng;
    Limits          limits;
    Collect&        next;

    std::expected<void, EngineError> restarted = std::unexpected(EngineError::Unsupported);
};

}  // namespace

TEST_CASE("core4_failed_start: a refused start keeps the search its terminal callback started") {
    EndsThenRefuses port;
    DeadlineEngine  eng{port, DeadlineEngine::Options{10ms}};
    Collect         next;
    Restart         first{eng, Limits{}, next};

    CHECK(eng.startSearch(pos(), Limits{}, first).error() == EngineError::TransportFailed);
    REQUIRE(first.restarted);
    REQUIRE(port.sink != nullptr);
    port.sink->onEvent(kMove);
    REQUIRE(next.get().size() == 1);
    CHECK(std::holds_alternative<BestMoveEvent>(next.get()[0]));
}

TEST_CASE("core4_failed_start: a refused start keeps the deadline of the newer search") {
    EndsThenRefuses port;
    DeadlineEngine  eng{port, DeadlineEngine::Options{10ms}};
    Collect         next;
    Restart         first{eng, turn(10ms), next};

    CHECK(eng.startSearch(pos(), Limits{}, first).error() == EngineError::TransportFailed);
    REQUIRE(first.restarted);
    REQUIRE(waitFor(next, 1));  // the engine never answers: the newer deadline must still fire
    CHECK(std::get<ErrorEvent>(next.get()[0]).message == "engine did not answer");
}

TEST_CASE("core4_failed_start: a nested start that is refused too leaves no state behind") {
    EndsThenRefuses port;
    port.refuseAfter = 1;
    DeadlineEngine eng{port, DeadlineEngine::Options{10ms}};
    Collect        next;
    Restart        first{eng, turn(10ms), next};

    CHECK(eng.startSearch(pos(), Limits{}, first).error() == EngineError::TransportFailed);
    CHECK(first.restarted.error() == EngineError::NotReady);
    std::this_thread::sleep_for(100ms);  // past the nested deadline: no stray timeout
    CHECK(next.get().empty());

    Collect last;
    REQUIRE(eng.startSearch(pos(), Limits{}, last));
    port.sink->onEvent(kMove);
    REQUIRE(last.get().size() == 1);
    CHECK(std::holds_alternative<BestMoveEvent>(last.get()[0]));
}
