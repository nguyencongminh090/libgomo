#include <doctest.h>

#include <thread>

#include "gomo/engine/blocking_search.hpp"

using namespace gomo;

namespace {

struct ScriptedEngine final : EnginePort {
    bool        threaded = false;
    std::thread worker;
    ~ScriptedEngine() override {
        if (worker.joinable()) worker.join();
    }

    bool                             supports(const Rules&) const override { return true; }
    std::expected<void, EngineError> setRules(const Rules&) override { return {}; }
    std::expected<void, EngineError> startSearch(const Position&, const Limits&,
                                                 EventSink& s) override {
        const bool late = !threaded;
        auto       emit = [&s, late] {
            InfoEvent i;
            i.depth = 3;
            s.onEvent(i);
            s.onEvent(UnparsedEvent{"noise"});
            i.depth = 4;
            s.onEvent(i);
            s.onEvent(BestMoveEvent{{7, 7}});
            // After the terminal event: must be ignored. Sync variant only: once search()
            // returns, its sink is destroyed, so a worker thread must not touch it (PORT-02).
            if (late) s.onEvent(InfoEvent{});
        };
        if (threaded) worker = std::thread(emit);
        else emit();
        return {};
    }
    void                             stop() override {}
    std::expected<void, EngineError> sendRaw(std::string_view) override { return {}; }
};

struct RefusingEngine final : EnginePort {
    bool                             supports(const Rules&) const override { return false; }
    std::expected<void, EngineError> setRules(const Rules&) override {
        return std::unexpected(EngineError::Unsupported);
    }
    std::expected<void, EngineError> startSearch(const Position&, const Limits&,
                                                 EventSink&) override {
        return std::unexpected(EngineError::NotReady);
    }
    void                             stop() override {}
    std::expected<void, EngineError> sendRaw(std::string_view) override { return {}; }
};

}  // namespace

TEST_CASE("port_blocking: returns best move and collected info, sync and threaded") {
    auto pos = Position::create(15, 15);
    REQUIRE(pos);
    for (bool threaded : {false, true}) {
        ScriptedEngine engine;
        engine.threaded = threaded;
        auto r          = search(engine, *pos, Limits{});
        REQUIRE(r);
        REQUIRE(std::holds_alternative<BestMoveEvent>(r->terminal));
        CHECK(std::get<BestMoveEvent>(r->terminal).coord == Coord{7, 7});
        REQUIRE(r->info.size() == 2);
        CHECK(r->info[0].depth == 3);
        CHECK(r->info[1].depth == 4);
        CHECK(r->other.size() == 1);
    }
}

TEST_CASE("port_blocking: a start error is returned, not waited on") {
    RefusingEngine engine;
    auto           pos = Position::create(15, 15);
    REQUIRE(pos);
    auto r = search(engine, *pos, Limits{});
    REQUIRE_FALSE(r);
    CHECK(r.error() == EngineError::NotReady);
}

TEST_CASE("ext02_blocking: a CustomEvent does not end the search and is kept in other") {
    struct CustomEngine final : EnginePort {
        bool                             supports(const Rules&) const override { return true; }
        std::expected<void, EngineError> setRules(const Rules&) override { return {}; }
        std::expected<void, EngineError> startSearch(const Position&, const Limits&,
                                                     EventSink& s) override {
            s.onEvent(CustomEvent{"vcn", {}});
            s.onEvent(BestMoveEvent{{7, 7}});
            return {};
        }
        void                             stop() override {}
        std::expected<void, EngineError> sendRaw(std::string_view) override { return {}; }
    };
    auto pos = Position::create(15, 15);
    REQUIRE(pos);
    CustomEngine engine;
    auto         r = search(engine, *pos, Limits{});
    REQUIRE(r);
    CHECK(std::holds_alternative<BestMoveEvent>(r->terminal));
    REQUIRE(r->other.size() == 1);
    CHECK(std::get<CustomEvent>(r->other[0]).tag == "vcn");
}
