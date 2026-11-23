#include <doctest.h>

#include <string>
#include <vector>

#include "gomo/core/engine_port.hpp"

using namespace gomo;

namespace {

struct RecordingSink final : EventSink {
    std::vector<SearchEvent> events;
    void                     onEvent(const SearchEvent& e) override { events.push_back(e); }
};

struct FakeEngine final : EnginePort {
    bool supports(const Rules& r) const override { return r.board == BoardRule::Freestyle; }
    std::expected<void, EngineError> setRules(const Rules& r) override {
        if (!supports(r)) return std::unexpected(EngineError::Unsupported);
        return {};
    }
    std::expected<void, EngineError> startSearch(const Position&, const Limits&,
                                                 EventSink& s) override {
        if (running) return std::unexpected(EngineError::Busy);
        running = true;
        sink    = &s;
        return {};
    }
    void                             stop() override { stopped = true; }
    std::expected<void, EngineError> sendRaw(std::string_view l) override {
        raw.emplace_back(l);
        return {};
    }
    bool                     running = false;
    bool                     stopped = false;
    EventSink*               sink    = nullptr;
    std::vector<std::string> raw;
};

}  // namespace

TEST_CASE("port_sink: events arrive in order") {
    RecordingSink sink;
    EventSink&    s = sink;
    s.onEvent(InfoEvent{});
    s.onEvent(BestMoveEvent{{1, 2}});
    s.onEvent(SwapEvent{});
    s.onEvent(PassEvent{});
    s.onEvent(ErrorEvent{"x"});
    s.onEvent(UnparsedEvent{"y"});
    REQUIRE(sink.events.size() == 6);
    CHECK(std::holds_alternative<InfoEvent>(sink.events[0]));
    CHECK(std::holds_alternative<BestMoveEvent>(sink.events[1]));
    CHECK(std::holds_alternative<SwapEvent>(sink.events[2]));
    CHECK(std::holds_alternative<PassEvent>(sink.events[3]));
    CHECK(std::holds_alternative<ErrorEvent>(sink.events[4]));
    CHECK(std::holds_alternative<UnparsedEvent>(sink.events[5]));
}

TEST_CASE("port_contract: a fake adapter is driven through the interface") {
    FakeEngine  fake;
    EnginePort& engine = fake;
    auto        pos    = Position::create(15, 15);
    REQUIRE(pos);
    RecordingSink sink;

    CHECK(engine.setRules(Rules{}).has_value());
    auto renju = Rules{BoardRule::Renju, OpeningRule::FreeOpen, std::nullopt};
    CHECK_FALSE(engine.supports(renju));
    CHECK(engine.setRules(renju).error() == EngineError::Unsupported);

    CHECK(engine.startSearch(*pos, Limits{}, sink).has_value());
    CHECK(engine.startSearch(*pos, Limits{}, sink).error() == EngineError::Busy);
    engine.stop();
    CHECK(fake.stopped);
    CHECK(engine.sendRaw("YXSHOWINFO").has_value());
    CHECK(fake.raw == std::vector<std::string>{"YXSHOWINFO"});
}
