#include <doctest.h>

#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "pygomo/engine/deadline_engine.hpp"
#include "pygomo/gtp/gtp_engine.hpp"
#include "pygomo/katagomo/katagomo_engine.hpp"

// DeadlineEngine over the real adapters with a fake transport (shapes as in the adapters' own
// tests).
using namespace pygomo;
using namespace std::chrono_literals;

namespace {

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
    void reply(std::string_view header) {
        say(header);
        say("");
    }  // GTP: header + blank terminator
    std::vector<std::string> lines() {
        std::lock_guard lock(m);
        return written;
    }

    std::mutex               m;
    std::vector<std::string> written;
    TransportListener*       listener = nullptr;
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

Position twoMoves() {
    auto p = Position::create(15, 15);
    REQUIRE(p);
    REQUIRE(p->play({7, 7}));  // H8
    REQUIRE(p->play({6, 8}));  // G7
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

std::string errorOf(const SearchEvent& e) { return std::get<ErrorEvent>(e).message; }

}  // namespace

TEST_CASE("dead2_gtp: a silent engine times out; the late genmove reply is dropped, "
          "the next search works") {
    FakeTransport t;
    GtpEngine     gtp{t};
    REQUIRE(gtp.open());
    DeadlineEngine eng{gtp, DeadlineEngine::Options{20ms}};
    gtp.stop();  // a no-op, but the decorator calls it too

    Collect a;
    // turnTime adds a maxTime line: ids 1..7, genmove is 7
    REQUIRE(eng.startSearch(twoMoves(), turn(20ms), a));
    REQUIRE(waitFor(a, 1));
    CHECK(errorOf(a.get()[0]) == "engine did not answer");

    Collect b;
    CHECK(eng.startSearch(twoMoves(), Limits{}, b).error() == EngineError::Busy);
    for (int id = 1; id < 7; ++id) t.reply("=" + std::to_string(id));
    t.reply("=7 J9");  // the late answer to the timed-out search
    CHECK(a.get().size() == 1);

    REQUIRE(eng.startSearch(twoMoves(), Limits{}, b));  // ids 8..13
    for (int id = 8; id < 13; ++id) t.reply("=" + std::to_string(id));
    t.reply("=7 J9");  // stale id of the earlier search: ignored by the adapter
    CHECK(b.get().empty());
    t.reply("=13 K10");
    REQUIRE(b.get().size() == 1);
    CHECK(std::holds_alternative<BestMoveEvent>(b.get()[0]));
}

TEST_CASE("dead2_gtp: a reply in time is delivered and no deadline event follows") {
    FakeTransport t;
    GtpEngine     gtp{t};
    REQUIRE(gtp.open());
    DeadlineEngine eng{gtp, DeadlineEngine::Options{20ms}};
    Collect        a;
    REQUIRE(eng.startSearch(twoMoves(), turn(20ms), a));
    for (int id = 1; id < 7; ++id) t.reply("=" + std::to_string(id));
    t.reply("=7 J9");
    std::this_thread::sleep_for(150ms);
    REQUIRE(a.get().size() == 1);
    CHECK(std::holds_alternative<BestMoveEvent>(a.get()[0]));
}

TEST_CASE("dead2_kata: a silent engine times out; the late move is dropped, "
          "the next search works") {
    FakeTransport  t;
    KataGomoEngine kata{t};
    REQUIRE(kata.open());
    DeadlineEngine eng{kata, DeadlineEngine::Options{20ms}};

    Collect a;
    REQUIRE(eng.startSearch(twoMoves(), turn(20ms), a));
    t.say("OK");
    t.say("MESSAGE Visits 10 Winrate 50%");
    REQUIRE(waitFor(a, 2));
    CHECK(errorOf(a.get()[1]) == "engine did not answer");

    Collect b;
    CHECK(eng.startSearch(twoMoves(), Limits{}, b).error() == EngineError::Busy);
    t.say("9,9");  // the late answer (no ids in this protocol): dropped
    CHECK(a.get().size() == 2);

    REQUIRE(eng.startSearch(twoMoves(), Limits{}, b));
    t.say("OK");
    t.say("10,10");
    REQUIRE(b.get().size() == 1);
    CHECK(std::get<BestMoveEvent>(b.get()[0]).coord == Coord{10, 10});
}

TEST_CASE("dead2_kata: an engine that answers in time gets no timeout event") {
    FakeTransport  t;
    KataGomoEngine kata{t};
    REQUIRE(kata.open());
    DeadlineEngine eng{kata, DeadlineEngine::Options{20ms}};
    Collect        a;
    REQUIRE(eng.startSearch(twoMoves(), turn(20ms), a));
    t.say("OK");
    t.say("9,9");
    std::this_thread::sleep_for(150ms);
    REQUIRE(a.get().size() == 1);
    CHECK(std::holds_alternative<BestMoveEvent>(a.get()[0]));
}
