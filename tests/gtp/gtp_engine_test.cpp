#include <doctest.h>

#include <functional>
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
    void close() override {}
    void say(std::string_view l) { listener->onLine(l); }
    /// One complete reply: header line, then the blank terminator.
    void reply(std::string_view header) {
        say(header);
        say("");
    }
    void                     closeNow() { listener->onClosed(CloseReason::Eof); }
    std::vector<std::string> lines() {
        std::lock_guard lock(m);
        return written;
    }

    std::mutex                              m;
    std::vector<std::string>                written;
    TransportListener*                      listener = nullptr;
    std::function<void(const std::string&)> onWrite;  // runs under the lock, after recording
    bool                                    failWrites = false;
};

struct Collect final : EventSink {
    void                     onEvent(const SearchEvent& e) override { events.push_back(e); }
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
    GtpEngine     e{t};
    Collect       sink;
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
    Rig  r;
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
        CHECK(r.sink.events == std::vector<SearchEvent>{ResignEvent{}});
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
    Rig   r;
    Rules renju;
    renju.board    = BoardRule::Renju;
    renju.maxMoves = 100;
    REQUIRE(r.e.supports(renju));
    REQUIRE(r.e.setRules(renju));
    Limits lim;
    lim.turnTime = std::chrono::milliseconds{2500};
    lim.maxNodes = 4000;
    auto empty   = Position::create(15, 15);
    REQUIRE(empty);
    REQUIRE(r.e.startSearch(*empty, lim, r.sink));
    CHECK(r.t.lines() == Lines{"1 boardsize 15 15", "2 clear_board", "3 RENJU", "4 maxmoves 100",
                               "5 kata-set-param maxTime 2.500", "6 kata-set-param maxVisits 4000",
                               "7 genmove B"});
}

TEST_CASE("gtp2_rules: what the protocol cannot say is Unsupported and nothing is sent") {
    Rig   r;
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
    Rig  r;
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
    r.t.reply("=2");                                   // late reply of the first search
    r.t.reply("=6 A1");                                // late genmove reply of the first search
    CHECK(r.sink.events.size() == 1);
    for (int id = 7; id < 12; ++id) r.t.reply("=" + std::to_string(id));
    r.t.reply("=12 H9");
    REQUIRE(r.sink.events.size() == 2);
    CHECK(r.sink.events[1] == SearchEvent{BestMoveEvent{Coord{7, 6}}});
}

TEST_CASE("gtp3_sequence: analyze mode sends kata-genmove_analyze with an interval; "
          "default stays genmove") {
    Rig r;
    r.e.setAnalyze(std::chrono::milliseconds{500});
    Limits lim;
    lim.maxNodes = 4000;
    REQUIRE(r.e.startSearch(twoMoves(), lim, r.sink));
    // Two moves played: Black to move.
    CHECK(r.t.lines().back() == "7 kata-genmove_analyze B interval 50");

    Rig d;
    REQUIRE(d.e.startSearch(twoMoves(), Limits{}, d.sink));
    CHECK(d.t.lines().back() == "6 genmove B");
}

TEST_CASE("gtp3_stream: info lines become InfoEvents while the reply is open, "
          "then play X ends it") {
    Rig r;
    r.e.setAnalyze(std::chrono::milliseconds{500});
    REQUIRE(r.e.startSearch(twoMoves(), Limits{}, r.sink));
    const int id = r.ackSetup(6);
    r.t.say("=" + std::to_string(id));  // printed before the first info line
    r.t.say("info move H8 visits 120 winrate 5600 prior 3100 order 0 pv H8 G7 J9");
    REQUIRE(r.sink.events.size() == 1);
    const auto& firstInfo = std::get<InfoEvent>(r.sink.events[0]);
    CHECK_FALSE(firstInfo.nodes);
    REQUIRE(firstInfo.candidates.size() == 1);
    CHECK(firstInfo.candidates[0].visits == 120);
    REQUIRE(firstInfo.candidates[0].winrate);
    CHECK(*firstInfo.candidates[0].winrate == doctest::Approx(0.56));
    r.t.say("info move H8 visits 240 winrate 5600 prior 3100 order 0 pv H8 G7 J9");
    CHECK(r.sink.events.size() == 2);  // search still running: no terminal event yet
    r.t.say("play H9");
    r.t.say("");
    REQUIRE(r.sink.events.size() == 3);
    CHECK(std::get<BestMoveEvent>(r.sink.events[2]).coord == Coord{7, 6});
    r.t.say("info move H8 visits 999 order 0 pv H8");  // outside a reply: dropped
    CHECK(r.sink.events.size() == 3);
}

TEST_CASE("gtp3_errors: ? reply, missing play, close") {
    {
        Rig r;
        r.e.setAnalyze(std::chrono::milliseconds{500});
        REQUIRE(r.e.startSearch(twoMoves(), Limits{}, r.sink));
        const int id = r.ackSetup(6);
        r.t.reply("?" + std::to_string(id) + " Could not parse genmove_analyze arguments");
        REQUIRE(r.sink.events.size() == 1);
        CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[0]));
    }
    {
        Rig r;
        r.e.setAnalyze(std::chrono::milliseconds{500});
        REQUIRE(r.e.startSearch(twoMoves(), Limits{}, r.sink));
        const int id = r.ackSetup(6);
        r.t.reply("=" + std::to_string(id) + " H9");  // not an analyze answer
        REQUIRE(r.sink.events.size() == 1);
        CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[0]));
    }
    {
        Rig r;
        r.e.setAnalyze(std::chrono::milliseconds{500});
        REQUIRE(r.e.startSearch(twoMoves(), Limits{}, r.sink));
        r.t.say("=6");
        r.t.say("info move H8 visits 1 order 0 pv H8");
        r.t.closeNow();
        REQUIRE(r.sink.events.size() == 2);
        CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[1]));
    }
}

TEST_CASE("gtp4_message: text outside a reply during a search is a MessageEvent") {
    SUBCASE("genmove") {
        Rig r;
        r.t.say("KataGo ready");  // before the search: dropped
        REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
        r.t.say("Loaded model, 8 threads");
        r.t.say("");  // a blank line outside a reply carries nothing
        const int id = r.ackSetup(6);
        r.t.reply("=" + std::to_string(id) + " K10");
        CHECK(r.sink.events == std::vector<SearchEvent>{MessageEvent{"Loaded model, 8 threads"},
                                                        BestMoveEvent{Coord{9, 5}}});
        r.t.say("late text");  // the search ended: dropped
        CHECK(r.sink.events.size() == 2);
    }
    SUBCASE("analyze, between replies") {
        Rig r;
        r.e.setAnalyze(std::chrono::milliseconds{500});
        REQUIRE(r.e.startSearch(twoMoves(), Limits{}, r.sink));
        r.t.reply("=1");
        r.t.reply("=2");
        r.t.say("some note");  // after a setup reply ended, before the next one starts
        for (int setup = 3; setup < 6; ++setup) r.t.reply("=" + std::to_string(setup));
        const int id = 6;
        r.t.say("=" + std::to_string(id));
        r.t.say("play H9");
        r.t.say("");
        CHECK(r.sink.events ==
              std::vector<SearchEvent>{MessageEvent{"some note"}, BestMoveEvent{Coord{7, 6}}});
    }
}

TEST_CASE("gtp5_text: free text starting with = or ? is a MessageEvent, not a reply header") {
    Rig r;
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    r.t.say("=== Loaded ===");
    r.t.say("??x");
    r.t.say("");
    const int id = r.ackSetup(6);
    r.t.reply("=" + std::to_string(id) + " K10");
    CHECK(r.sink.events == std::vector<SearchEvent>{MessageEvent{"=== Loaded ==="},
                                                    MessageEvent{"??x"},
                                                    BestMoveEvent{Coord{9, 5}}});
}

TEST_CASE("gtp6_raw: the reply to a raw command sent before a search does not end the search") {
    Rig r;
    REQUIRE(r.e.sendRaw("kata-set-param maxVisits 5"));
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    r.t.reply("=");  // the raw command's reply comes before the search's own replies
    const int id = r.ackSetup(6);
    r.t.reply("=" + std::to_string(id) + " K10");
    CHECK(r.sink.events == std::vector<SearchEvent>{MessageEvent{"="}, BestMoveEvent{Coord{9, 5}}});
}

TEST_CASE("gtp6_raw: raw replies read during a search are MessageEvents, also a ? reply") {
    Rig r;
    REQUIRE(r.e.sendRaw("kata-set-param maxVisits 5"));
    REQUIRE(r.e.sendRaw("kata-unknown"));
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    r.t.reply("= ok");  // written before the search's lines, so answered first
    r.t.reply("? unknown command");
    CHECK(r.sink.events.size() == 2);  // the search still runs
    const int id = r.ackSetup(6);
    r.t.reply("=" + std::to_string(id) + " K10");
    CHECK(r.sink.events == std::vector<SearchEvent>{MessageEvent{"= ok"},
                                                    MessageEvent{"? unknown command"},
                                                    BestMoveEvent{Coord{9, 5}}});
}

TEST_CASE("gtp6_raw: a raw command sent during setup does not hide an id-less setup failure") {
    Rig r;
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    REQUIRE(r.e.sendRaw("kata-set-param maxVisits 5"));  // queued after genmove
    r.t.reply("=1");
    r.t.reply("=2");
    r.t.reply("? bad setup");  // the reply to line 3, from an engine that drops ids
    REQUIRE(r.sink.events.size() == 1);
    CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[0]));
}

TEST_CASE("gtp6_raw: a raw reply after the genmove reply is dropped; the next search is clean") {
    Rig r;
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    REQUIRE(r.e.sendRaw("kata-set-param maxVisits 5"));
    const int id = r.ackSetup(6);
    r.t.reply("=" + std::to_string(id) + " K10");
    r.t.reply("=");  // the raw reply, after the search ended
    REQUIRE(r.sink.events == std::vector<SearchEvent>{BestMoveEvent{Coord{9, 5}}});
    Collect next;
    REQUIRE(r.e.startSearch(twoMoves(), {}, next));
    r.t.reply("? bad setup");
    REQUIRE(next.events.size() == 1);
    CHECK(std::holds_alternative<ErrorEvent>(next.events[0]));
}

TEST_CASE("gtp6_raw: a raw line with its own id inside the search's id range") {
    Rig r;
    REQUIRE(r.e.sendRaw("6 name"));  // 6 will also be the genmove id
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    r.t.reply("=6 KataGo");
    CHECK(r.sink.events == std::vector<SearchEvent>{MessageEvent{"=6 KataGo"}});
    r.t.reply("=1");  // the search's replies follow
    const int id = r.ackSetup(6);
    r.t.reply("=" + std::to_string(id) + " K10");
    CHECK(r.sink.events.back() == SearchEvent{BestMoveEvent{Coord{9, 5}}});
}

TEST_CASE("gtp6_raw: a failed raw write leaves no reply expected") {
    Rig r;
    r.t.failWrites = true;
    CHECK(r.e.sendRaw("kata-set-param maxVisits 5") ==
          std::unexpected(EngineError::TransportFailed));
    r.t.failWrites = false;
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    r.t.reply("? bad setup");  // nothing queued for the failed write: this is the search's
    REQUIRE(r.sink.events.size() == 1);
    CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[0]));
}

TEST_CASE("gtp6_raw: an id-less reply with no raw command pending still ends the search") {
    SUBCASE("? reply") {
        Rig r;
        REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
        r.t.reply("? bad setup");
        REQUIRE(r.sink.events.size() == 1);
        CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[0]));
    }
    SUBCASE("= reply") {
        Rig r;
        REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
        r.t.reply("=");
        REQUIRE(r.sink.events.size() == 1);
        CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[0]));
    }
    SUBCASE("empty and comment lines get no reply and are not expected") {
        Rig r;
        REQUIRE(r.e.sendRaw(""));
        REQUIRE(r.e.sendRaw("# a comment"));
        REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
        r.t.reply("? bad setup");
        REQUIRE(r.sink.events.size() == 1);
        CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[0]));
    }
}

TEST_CASE("gtp6_raw: a raw reply between searches is counted and dropped") {
    Rig r;
    REQUIRE(r.e.sendRaw("kata-set-param maxVisits 5"));
    r.t.reply("= ok");  // no search runs: dropped, but no longer pending
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    r.t.reply("? bad setup");
    REQUIRE(r.sink.events.size() == 1);
    CHECK(std::holds_alternative<ErrorEvent>(r.sink.events[0]));
}

TEST_CASE("gtp6_raw: a raw reply whose header was read before the search started") {
    Rig r;
    REQUIRE(r.e.sendRaw("kata-set-param maxVisits 5"));
    r.t.say("= ok");
    REQUIRE(r.e.startSearch(twoMoves(), {}, r.sink));
    r.t.say("");  // ends the raw reply, which started before the search
    const int id = r.ackSetup(6);
    r.t.reply("=" + std::to_string(id) + " K10");
    CHECK(r.sink.events ==
          std::vector<SearchEvent>{MessageEvent{"= ok"}, BestMoveEvent{Coord{9, 5}}});
}

TEST_CASE("port3_gtp: a close during startSearch gives one outcome, not event plus error") {
    Rig r;
    r.t.onWrite = [&](const std::string& l) {
        if (!l.starts_with("2 ")) return;
        r.t.failWrites = true;
        r.t.closeNow();  // the reader sees EOF while startSearch is still writing
    };
    CHECK(r.e.startSearch(twoMoves(), {}, r.sink));
    REQUIRE(r.sink.events.size() == 1);
    CHECK(r.sink.events[0] == SearchEvent{ErrorEvent{"engine connection closed"}});
}

TEST_CASE("port3_gtp: a setup reply that ends the search before a failed write gives one outcome") {
    Rig r;
    r.t.onWrite = [&](const std::string& l) {
        if (l.starts_with("1 ")) r.t.reply("?1 unacceptable size");
        if (l.starts_with("2 ")) r.t.failWrites = true;
    };
    CHECK(r.e.startSearch(twoMoves(), {}, r.sink));
    REQUIRE(r.sink.events.size() == 1);
    CHECK(r.sink.events[0] == SearchEvent{ErrorEvent{"unacceptable size"}});
}

TEST_CASE("port3_gtp: a failed write while the search is live is refused with no event") {
    Rig r;
    r.t.onWrite = [&](const std::string& l) { r.t.failWrites = l.starts_with("2 "); };
    CHECK(r.e.startSearch(twoMoves(), {}, r.sink).error() == EngineError::TransportFailed);
    CHECK(r.sink.events.empty());
    r.t.onWrite    = nullptr;
    r.t.failWrites = false;
    CHECK(r.e.startSearch(twoMoves(), {}, r.sink));  // the refused search left no sink
}

TEST_CASE("gtp7_engine: analyze streams multi-candidate kata InfoEvent and ends with play") {
    Rig r;
    r.e.setAnalyze(std::chrono::milliseconds{500});
    REQUIRE(r.e.startSearch(twoMoves(), Limits{}, r.sink));
    const int id = r.ackSetup(6);
    r.t.say("=" + std::to_string(id));
    r.t.say("info move H8 visits 120 utility 0.12 winrate 0.56 scoreMean 20 scoreLead 20 "
            "order 0 pv H8 G7 pvVisits 120 60 "
            "info move J9 visits 40 utility 0.02 winrate 0.51 order 1 pv J9");
    REQUIRE(r.sink.events.size() == 1);
    const auto& info = std::get<InfoEvent>(r.sink.events[0]);
    CHECK_FALSE(info.nodes);
    CHECK(info.principalVariation == std::vector<Coord>{{7, 7}, {6, 8}});
    REQUIRE(info.candidates.size() == 2);
    CHECK(info.candidates[0].rank == 1);
    CHECK(info.candidates[0].visits == 120);
    REQUIRE(info.candidates[0].winrate);
    CHECK(*info.candidates[0].winrate == doctest::Approx(0.56));
    CHECK_FALSE(info.candidates[0].drawrate);
    CHECK(info.candidates[1].rank == 2);
    CHECK(info.candidates[1].move == Coord{8, 6});
    CHECK(info.candidates[1].visits == 40);
    REQUIRE(info.candidates[1].winrate);
    CHECK(*info.candidates[1].winrate == doctest::Approx(0.51));
    r.t.say("play J9");
    r.t.say("");
    REQUIRE(r.sink.events.size() == 2);
    CHECK(std::get<BestMoveEvent>(r.sink.events[1]).coord == Coord{8, 6});
}

TEST_CASE("ext04_gtp: custom parser handles analyze info and free text; default unchanged") {
    auto parser = [](std::string_view l) -> SearchEvent {
        if (l.starts_with("NOTE ")) {
            return CustomEvent{"note", {{"text", std::string{l.substr(5)}}}};
        }
        if (l.starts_with("info ")) return CustomEvent{"info", {{"raw", std::string{l}}}};
        return UnparsedEvent{std::string{l}};
    };

    FakeTransport t;
    GtpEngine     e{t, parser};
    Collect       sink;
    REQUIRE(e.open());
    e.setAnalyze(std::chrono::milliseconds{500});
    REQUIRE(e.startSearch(twoMoves(), Limits{}, sink));
    t.say("NOTE warmup");
    for (int id = 1; id < 6; ++id) t.reply("=" + std::to_string(id));
    t.say("=6");
    t.say("info move H8 visits 120 order 0 pv H8");
    t.say("play H9");
    t.say("");
    REQUIRE(sink.events.size() == 3);
    CHECK(std::get<CustomEvent>(sink.events[0]).tag == "note");
    CHECK(std::get<CustomEvent>(sink.events[1]).tag == "info");
    CHECK(std::get<BestMoveEvent>(sink.events[2]).coord == Coord{7, 6});

    Rig r;  // default constructor argument
    r.e.setAnalyze(std::chrono::milliseconds{500});
    REQUIRE(r.e.startSearch(twoMoves(), Limits{}, r.sink));
    r.t.say("NOTE warmup");
    r.ackSetup(6);
    r.t.say("=6");
    r.t.say("info move H8 visits 120 order 0 pv H8");
    r.t.say("play H9");
    r.t.say("");
    REQUIRE(r.sink.events.size() == 3);
    CHECK(std::get<MessageEvent>(r.sink.events[0]).text == "NOTE warmup");
    CHECK(std::holds_alternative<InfoEvent>(r.sink.events[1]));
    CHECK(std::get<BestMoveEvent>(r.sink.events[2]).coord == Coord{7, 6});
}

TEST_CASE("ext04_gtp: reply headers, blank terminators and lines outside a search are skipped") {
    Lines seen;
    auto  parser = [&](std::string_view l) -> SearchEvent {
        seen.emplace_back(l);
        return UnparsedEvent{std::string{l}};
    };

    FakeTransport t;
    GtpEngine     e{t, parser};
    Collect       sink;
    REQUIRE(e.open());
    t.say("banner");  // before search: skipped
    REQUIRE(e.startSearch(twoMoves(), Limits{}, sink));
    t.say("free line");
    t.say("");  // blank line outside a reply: skipped
    t.say("=1 first");
    t.say("body line");
    t.say("");
    for (int id = 2; id < 6; ++id) t.reply("=" + std::to_string(id));
    t.reply("=6 H9");
    t.say("after");  // after search: skipped
    CHECK(seen == Lines{"free line", "body line"});
    CHECK(sink.events ==
          std::vector<SearchEvent>{MessageEvent{"free line"}, BestMoveEvent{Coord{7, 6}}});
}

TEST_CASE("ext04_gtp: a terminal event from the custom parser is ignored") {
    auto parser = [](std::string_view l) -> SearchEvent {
        if (l == "NOTE oops") return ErrorEvent{"ignored"};
        if (l == "play H9") return BestMoveEvent{{0, 0}};
        return UnparsedEvent{std::string{l}};
    };

    FakeTransport t;
    GtpEngine     e{t, parser};
    Collect       sink;
    REQUIRE(e.open());
    e.setAnalyze(std::chrono::milliseconds{500});
    REQUIRE(e.startSearch(twoMoves(), Limits{}, sink));
    t.say("NOTE oops");
    for (int id = 1; id < 6; ++id) t.reply("=" + std::to_string(id));
    t.say("=6");
    t.say("play H9");
    CHECK(sink.events.size() == 1);  // search still open until the blank terminator
    t.say("");
    CHECK(sink.events ==
          std::vector<SearchEvent>{MessageEvent{"NOTE oops"}, BestMoveEvent{Coord{7, 6}}});
}

TEST_CASE("ext04_gtp: the body of a stale reply of an earlier search is not parsed") {
    Lines seen;
    auto  parser = [&](std::string_view l) -> SearchEvent {
        seen.emplace_back(l);
        return CustomEvent{"seen", {}};
    };

    FakeTransport t;
    GtpEngine     e{t, parser};
    Collect       first;
    Collect       second;
    REQUIRE(e.open());
    REQUIRE(e.startSearch(twoMoves(), Limits{}, first));
    t.reply("?1 refused");  // ends the first search; its ids 2..6 are now stale
    REQUIRE(e.startSearch(twoMoves(), Limits{}, second));
    t.say("=2 old");
    t.say("stale body");
    t.say("");
    CHECK(seen.empty());
    CHECK(second.events.empty());
}

TEST_CASE("ext05_gtp: rules with extensions are unsupported and write nothing") {
    Rig         r;
    const Rules rules{.extensions = {{"custom-rule", true}}};
    CHECK_FALSE(r.e.supports(rules));
    const auto result = r.e.setRules(rules);
    REQUIRE_FALSE(result);
    CHECK(result.error() == EngineError::Unsupported);
    CHECK(r.t.lines().empty());
}
