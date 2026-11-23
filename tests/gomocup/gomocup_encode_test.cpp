#include <doctest.h>

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "pygomo/gomocup/gomocup_encode.hpp"
#include "pygomo/gomocup/gomocup_engine.hpp"

using namespace pygomo;

namespace {

using Lines = std::vector<std::string>;

/// Records what the adapter writes.
class RecordingTransport final : public Transport {
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
    void  close() override {}
    void  say(std::string_view l) { listener->onLine(l); }
    Lines lines() {
        std::lock_guard lock(m);
        return written;
    }

    std::mutex         m;
    Lines              written;
    TransportListener* listener = nullptr;
};

struct NullSink final : EventSink {
    void onEvent(const SearchEvent&) override {}
};

Position withMoves(std::vector<Coord> moves, int w = 15, int h = 15, BoardLayout layout = {}) {
    auto p = Position::create(w, h, std::move(layout));
    REQUIRE(p);
    for (Coord c : moves) REQUIRE(p->play(c));
    return *p;
}

Limits allLimits() {
    Limits lim;
    lim.turnTime  = std::chrono::milliseconds{5000};
    lim.matchTime = std::chrono::milliseconds{90000};
    lim.maxDepth  = 12;
    lim.maxNodes  = 1000000;
    return lim;
}

// A Rapfi-style encoder an application could write: it sends the Caro rule, walls, torus and
// portals, sets the position with YXBOARD and starts the search with YXNBEST.
std::expected<Lines, EngineError> fakeRapfi(const SearchRequest& req) {
    const Position&    pos    = req.position;
    const BoardLayout& layout = pos.layout();
    if (pos.width() != pos.height()) return std::unexpected(EngineError::Unsupported);
    Lines lines{"START " + std::to_string(pos.width())};
    if (auto it = req.rules.extensions.find("rapfi.rule"); it != req.rules.extensions.end()) {
        const auto* id = std::get_if<std::int64_t>(&it->second);
        if (!id) return std::unexpected(EngineError::Unsupported);
        lines.push_back("INFO RULE " + std::to_string(*id));
    }
    Lines walls;
    for (Coord c : layout.blocked) {
        lines.push_back("INFO WALL " + format(c));
        walls.push_back(format(c) + ",3");
    }
    if (layout.torus) lines.emplace_back("INFO TORUS 1");
    for (const auto& [a, b] : layout.portals) {
        lines.push_back("INFO YXPORTAL " + format(a) + " " + format(b));
    }
    auto block = encode::boardBlock("YXBOARD", pos.moves(), walls);
    lines.insert(lines.end(), block.begin(), block.end());
    lines.emplace_back("YXNBEST 1");
    return lines;
}

}  // namespace

TEST_CASE("ext07_default: the engine writes the same script byte for byte") {
    {
        RecordingTransport t;
        GomocupEngine      e{t};
        REQUIRE(e.open());
        NullSink sink;
        REQUIRE(e.setRules(Rules{BoardRule::Renju, OpeningRule::FreeOpen, 200}));
        REQUIRE(e.startSearch(withMoves({}), allLimits(), sink));
        CHECK(t.lines() == Lines{"START 15", "INFO rule 4", "INFO max_moves 200",
                                 "INFO timeout_turn 5000", "INFO timeout_match 90000",
                                 "INFO max_depth 12", "INFO max_node 1000000", "BEGIN"});
    }
    {
        RecordingTransport t;
        GomocupEngine      e{t};
        REQUIRE(e.open());
        NullSink sink;
        REQUIRE(e.startSearch(withMoves({{7, 7}, {8, 7}}, 20, 20), Limits{}, sink));
        // Two moves played, so Black is to move: Black's stone (7,7) is flag 1.
        CHECK(t.lines() == Lines{"START 20", "INFO rule 0", "BOARD", "7,7,1", "8,7,2", "DONE"});
    }
    {
        RecordingTransport t;
        GomocupEngine      e{t};
        REQUIRE(e.open());
        NullSink sink;
        REQUIRE(e.setRules(Rules{BoardRule::Freestyle, OpeningRule::Swap1, std::nullopt}));
        Limits lim;
        lim.matchTime = std::chrono::milliseconds{0};
        REQUIRE(e.startSearch(withMoves({{0, 0}}), lim, sink));
        CHECK(t.lines() ==
              Lines{"START 15", "INFO rule 5", "INFO timeout_match 0", "BOARD", "0,0,2", "DONE"});
    }
}

TEST_CASE("ext07_default: defaultGomocupRequest builds the script and refuses what it cannot") {
    const Rules rules{BoardRule::Standard, OpeningRule::FreeOpen, 100};
    auto        ok = encode::defaultGomocupRequest(
        SearchRequest{rules, withMoves({{1, 2}, {3, 4}, {5, 6}}), allLimits()});
    REQUIRE(ok);
    CHECK(*ok == Lines{"START 15", "INFO rule 1", "INFO max_moves 100", "INFO timeout_turn 5000",
                       "INFO timeout_match 90000", "INFO max_depth 12", "INFO max_node 1000000",
                       "BOARD", "1,2,2", "3,4,1", "5,6,2", "DONE"});

    auto empty = encode::defaultGomocupRequest(SearchRequest{Rules{}, withMoves({}), Limits{}});
    REQUIRE(empty);
    CHECK(*empty == Lines{"START 15", "INFO rule 0", "BEGIN"});

    const auto refused = [](const SearchRequest& req) {
        auto r = encode::defaultGomocupRequest(req);
        return !r && r.error() == EngineError::Unsupported;
    };
    CHECK(refused(SearchRequest{Rules{BoardRule::Renju, OpeningRule::Swap2, std::nullopt},
                                withMoves({}), Limits{}}));
    CHECK(refused(SearchRequest{Rules{.extensions = {{"rapfi.rule", std::int64_t{8}}}},
                                withMoves({}), Limits{}}));
    CHECK(refused(
        SearchRequest{Rules{}, withMoves({}, 15, 15, BoardLayout{.torus = true}), Limits{}}));
}

TEST_CASE("ext07_custom: a fake Rapfi encoder replaces the whole script") {
    RecordingTransport t;
    GomocupEngine      e{t, {}, fakeRapfi};
    REQUIRE(e.open());
    const Rules caro{.extensions = {{"rapfi.rule", std::int64_t{8}}}};
    CHECK(e.supports(caro));
    REQUIRE(e.setRules(caro));

    BoardLayout layout{
        .torus = true, .blocked = {{0, 0}, {14, 3}}, .portals = {{Coord{1, 1}, Coord{13, 13}}}};
    NullSink sink;
    REQUIRE(e.startSearch(withMoves({{7, 7}, {8, 7}, {7, 8}}, 15, 15, layout), Limits{}, sink));
    CHECK(t.lines() == Lines{"START 15", "INFO RULE 8", "INFO WALL 0,0", "INFO WALL 14,3",
                             "INFO TORUS 1", "INFO YXPORTAL 1,1 13,13", "YXBOARD", "7,7,2", "8,7,1",
                             "7,8,2", "0,0,3", "14,3,3", "DONE", "YXNBEST 1"});
    t.say("6,6");  // the search ends normally with a legal move
    CHECK(e.startSearch(withMoves({}), Limits{}, sink));
}

TEST_CASE("ext07_custom: supports() accepts any valid rules once an encoder is installed") {
    RecordingTransport t;
    GomocupEngine      e{t, {}, fakeRapfi};
    CHECK(e.supports(Rules{BoardRule::Standard, OpeningRule::Swap1, std::nullopt}));
    CHECK(e.supports(Rules{.extensions = {{"anything", true}}}));
    CHECK_FALSE(e.supports(Rules{.maxMoves = 0}));
    CHECK(e.setRules(Rules{.maxMoves = 0}).error() == EngineError::Unsupported);
}

TEST_CASE("ext07_refuse: without an encoder, extensions and layouts send nothing") {
    RecordingTransport t;
    GomocupEngine      e{t};
    REQUIRE(e.open());
    NullSink sink;
    CHECK(e.setRules(Rules{.extensions = {{"rapfi.rule", std::int64_t{8}}}}).error() ==
          EngineError::Unsupported);

    const BoardLayout walls{.blocked = {{0, 0}}};
    auto              r = e.startSearch(withMoves({{7, 7}}, 15, 15, walls), Limits{}, sink);
    REQUIRE_FALSE(r);
    CHECK(r.error() == EngineError::Unsupported);
    CHECK(t.lines().empty());

    // Not Busy: a normal search still starts.
    REQUIRE(e.startSearch(withMoves({}), Limits{}, sink));
    CHECK(t.lines() == Lines{"START 15", "INFO rule 0", "BEGIN"});
}

TEST_CASE("ext07_refuse: an encoder error sends nothing and leaves the engine idle") {
    RecordingTransport t;
    bool               refuse = true;
    GomocupEngine e{t, {}, [&refuse](const SearchRequest&) -> std::expected<Lines, EngineError> {
                        if (refuse) return std::unexpected(EngineError::Unsupported);
                        return Lines{"BEGIN"};
                    }};
    REQUIRE(e.open());
    NullSink sink;
    auto     r = e.startSearch(withMoves({}), Limits{}, sink);
    REQUIRE_FALSE(r);
    CHECK(r.error() == EngineError::Unsupported);
    CHECK(t.lines().empty());

    refuse = false;
    REQUIRE(e.startSearch(withMoves({}), Limits{}, sink));
    CHECK(t.lines() == Lines{"BEGIN"});
}

TEST_CASE("ext07_board_block: colours follow the side to move; extra lines come before DONE") {
    const std::vector<Move> odd{
        {{7, 7}, Side::Black}, {{8, 7}, Side::White}, {{7, 8}, Side::Black}};
    // Three moves: White is to move, so White's stone is 1.
    CHECK(encode::boardBlock("BOARD", odd) == Lines{"BOARD", "7,7,2", "8,7,1", "7,8,2", "DONE"});

    const std::vector<Move> even{{{7, 7}, Side::Black}, {{8, 7}, Side::White}};
    const Lines             extra{"0,0,3", "1,0,3"};
    CHECK(encode::boardBlock("YXBOARD", even, extra) ==
          Lines{"YXBOARD", "7,7,1", "8,7,2", "0,0,3", "1,0,3", "DONE"});

    CHECK(encode::boardBlock("BOARD", {}) == Lines{"BOARD", "DONE"});
}
