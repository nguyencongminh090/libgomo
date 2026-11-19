#include <doctest.h>

#include <mutex>
#include <string>
#include <vector>

#include "pygomo/gtp/gtp_engine.hpp"

using namespace pygomo;

namespace {

/// Records what the adapter writes; the test plays the engine by calling say()/closeNow().
/// Reply shapes mirror KataGomo Gom2026 cpp/command/gtp.cpp (source-derived, no real engine).
class FakeTransport final : public Transport {
public:
    std::expected<void, TransportError> start(TransportListener& l) override { listener = &l; return {}; }
    std::expected<void, TransportError> writeLine(std::string_view line) override {
        std::lock_guard lock(m);
        written.emplace_back(line);
        return {};
    }
    void close() override {}
    void say(std::string_view l) { listener->onLine(l); }
    /// One complete reply: header line, then the blank terminator.
    void reply(std::string_view header) { say(header); say(""); }
    void closeNow() { listener->onClosed(CloseReason::Eof); }
    std::vector<std::string> lines() { std::lock_guard lock(m); return written; }

    std::mutex m;
    std::vector<std::string> written;
    TransportListener* listener = nullptr;
};

struct Collect final : EventSink {
    void onEvent(const SearchEvent& e) override { events.push_back(e); }
    std::vector<SearchEvent> events;
};

Position twoMoves(int w = 15, int h = 15) {
    auto p = Position::create(w, h);
    REQUIRE(p);
    REQUIRE(p->play({7, 7}));  // H8
    REQUIRE(p->play({6, 8}));  // G7
    return *p;
}

struct Rig {
    FakeTransport t;
    GtpEngine e{t};
    Collect sink;
    Rig() { REQUIRE(e.open()); }
    /// Answers every setup command of the first search with "=<id>" and returns the genmove id.
    int ackSetup(int lastId) {
        for (int id = 1; id < lastId; ++id) t.reply("=" + std::to_string(id));
        return lastId;
    }
};

using Lines = std::vector<std::string>;

}  // namespace

TEST_CASE("gtp2_sequence: boardsize, clear_board, rule, plays, genmove with ids") {
    Rig r;
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    CHECK(r.t.lines() == Lines{"1 boardsize 15 15", "2 clear_board", "3 FREESTYLE", "4 play B H8",
                               "5 play W G7", "6 genmove B"});
}

TEST_CASE("gtp2_sequence: an empty board asks the first side to move; ids keep growing") {
    Rig r;
    auto empty = Position::create(15, 15);
    REQUIRE(empty);
    REQUIRE(r.e.startSearch(*empty, {}, r.sink));
    r.t.reply("=1");
    r.t.reply("=2");
    r.t.reply("=3");
    r.t.reply("=4 H8");
    REQUIRE(r.sink.events.size() == 1);
    REQUIRE(r.e.startSearch(*empty, {}, r.sink));
    CHECK(r.t.lines().back() == "8 genmove B");
}

TEST_CASE("gtp2_result: a vertex becomes the best move") {
    Rig r;
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    r.ackSetup(6);
    r.t.reply("=6 J9");
    REQUIRE(r.sink.events.size() == 1);
    CHECK(r.sink.events[0] == SearchEvent{BestMoveEvent{Coord{8, 6}}});
}

TEST_CASE("gtp2_result: pass, resign, an error reply and an unreadable move end the search once") {
    SUBCASE("pass") {
        Rig r;
        REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
        r.ackSetup(6);
        r.t.reply("=6 pass");
        CHECK(r.sink.events == std::vector<SearchEvent>{PassEvent{}});
    }
    SUBCASE("resign") {
        Rig r;
        REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
        r.ackSetup(6);
        r.t.reply("=6 resign");
        CHECK(r.sink.events == std::vector<SearchEvent>{ErrorEvent{"engine resigned"}});
    }
    SUBCASE("an error reply to a setup command, later replies ignored") {
        Rig r;
        REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
        r.t.reply("=1");
        r.t.reply("=2");
        r.t.reply("=3");
        r.t.reply("?4 illegal move");
        r.t.reply("=5");
        r.t.reply("=6 H9");
        CHECK(r.sink.events == std::vector<SearchEvent>{ErrorEvent{"illegal move"}});
    }
    SUBCASE("unreadable move") {
        Rig r;
        REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
        r.ackSetup(6);
        r.t.reply("=6 Z99");
        REQUIRE(r.sink.events.size() == 1);
        CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[0]));
    }
    SUBCASE("occupied point") {
        Rig r;
        REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
        r.ackSetup(6);
        r.t.reply("=6 H8");
        REQUIRE(r.sink.events.size() == 1);
        CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[0]));
    }
}

TEST_CASE("gtp2_result: replies with body lines, banners and stale ids") {
    Rig r;
    r.t.say("KataGo ready");  // outside a reply, dropped
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    r.t.say("=1 first line");
    r.t.say("second line");
    r.t.say("");
    r.ackSetup(6);
    r.t.reply("=99 A1");  // not an id of this search
    CHECK(r.sink.events.empty());
    r.t.reply("=6 K10");
    CHECK(r.sink.events == std::vector<SearchEvent>{BestMoveEvent{Coord{9, 5}}});
}

TEST_CASE("gtp2_rules: supported rules and limits become commands") {
    Rig r;
    Rules renju;
    renju.board = BoardRule::Renju;
    renju.maxMoves = 100;
    REQUIRE(r.e.supports(renju));
    REQUIRE(r.e.setRules(renju));
    Limits lim;
    lim.turnTime = std::chrono::milliseconds{2500};
    lim.maxNodes = 4000;
    auto empty = Position::create(15, 15);
    REQUIRE(empty);
    REQUIRE(r.e.startSearch(*empty, lim, r.sink));
    CHECK(r.t.lines() == Lines{"1 boardsize 15 15", "2 clear_board", "3 RENJU", "4 maxmoves 100",
                               "5 kata-set-param maxTime 2.500", "6 kata-set-param maxVisits 4000",
                               "7 genmove B"});
}

TEST_CASE("gtp2_rules: what the protocol cannot say is Unsupported and nothing is sent") {
    Rig r;
    Rules swap;
    swap.opening = OpeningRule::Swap1;
    CHECK_FALSE(r.e.supports(swap));
    CHECK(r.e.setRules(swap).error() == EngineError::Unsupported);

    Limits match;
    match.matchTime = std::chrono::milliseconds{60000};
    CHECK(r.e.startSearch(twoMoves(), match, r.sink).error() == EngineError::Unsupported);
    Limits depth;
    depth.maxDepth = 8;
    CHECK(r.e.startSearch(twoMoves(), depth, r.sink).error() == EngineError::Unsupported);
    CHECK(r.t.lines().empty());
}

TEST_CASE("gtp2_rules: a rectangular board") {
    Rig r;
    auto p = Position::create(20, 10);
    REQUIRE(p);
    REQUIRE(p->play({19, 0}));  // U10 (columns skip I)
    REQUIRE(r.e.startSearch(*p, {}, r.sink));
    CHECK(r.t.lines() == Lines{"1 boardsize 20 10", "2 clear_board", "3 FREESTYLE", "4 play B U10",
                               "5 genmove W"});
}

TEST_CASE("gtp2_lifecycle: Busy, stop writes nothing, close ends the search once") {
    Rig r;
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    CHECK(r.e.startSearch(twoMoves(), {}, r.sink).error() == EngineError::Busy);
    const auto before = r.t.lines().size();
    r.e.stop();
    CHECK(r.t.lines().size() == before);

    r.t.closeNow();
    REQUIRE(r.sink.events.size() == 1);
    CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[0]));
    CHECK(r.e.startSearch(twoMoves(), {}, r.sink).error() == EngineError::NotReady);
}

TEST_CASE("gtp2_lifecycle: stop during a search does not end it; the move still arrives") {
    Rig r;
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    r.e.stop();
    r.ackSetup(6);
    CHECK(r.sink.events.empty());
    r.t.reply("=6 H9");
    CHECK(r.sink.events == std::vector<SearchEvent>{BestMoveEvent{Coord{7, 6}}});
    r.e.stop();  // harmless when no search runs
}

TEST_CASE("gtp2_lifecycle: a new search after an error ignores the old search's late replies") {
    Rig r;
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    r.t.reply("?1 unacceptable size");
    REQUIRE(r.sink.events.size() == 1);
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));  // ids 7..12
    r.t.reply("=2");   // late reply of the first search
    r.t.reply("=6 A1");  // late genmove reply of the first search
    CHECK(r.sink.events.size() == 1);
    for (int id = 7; id < 12; ++id) r.t.reply("=" + std::to_string(id));
    r.t.reply("=12 H9");
    REQUIRE(r.sink.events.size() == 2);
    CHECK(r.sink.events[1] == SearchEvent{BestMoveEvent{Coord{7, 6}}});
}
