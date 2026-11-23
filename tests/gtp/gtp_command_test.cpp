// EXT-10: CommandPort on GtpEngine: one-line None commands, force waits for genmove's reply.
#include <doctest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "../support/command_fakes.hpp"
#include "pygomo/gtp/gtp_engine.hpp"

using namespace pygomo;
using namespace pygomo::testing;
using namespace std::chrono_literals;

namespace {

Position emptyBoard() {
    auto p = Position::create(15, 15);
    REQUIRE(p);
    return *p;
}

Command setParam() {
    return Command{.name = "kata-set-param", .args = {std::string{"maxVisits"}, std::int64_t{100}}};
}

struct Rig {
    Log           log;
    FakeTransport t{log};
    GtpEngine     e{t};
    Rig() { REQUIRE(e.open()); }
    void reply(std::string_view header) {
        t.say(header);
        t.say("");
    }
};

}  // namespace

TEST_CASE("ext10_gtp: a None command is written as is; other forms are Unsupported") {
    Rig     r;
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(setParam(), cmd));
    CHECK(r.t.lines() == Lines{"kata-set-param maxVisits 100"});

    Command search{.name = "kata-genmove_analyze", .completion = Command::Search{}};
    Command until{.name       = "x",
                  .completion = Command::Until{[](const SearchEvent&) { return true; }}};
    Command block{.name = "x", .block = {Coord{1, 1}}};
    Command terminated{.name = "x", .terminator = "DONE"};
    for (const Command& c : {search, until, block, terminated}) {
        CHECK(r.e.execute(c, cmd).error() == EngineError::Unsupported);
    }
    CHECK(r.t.lines().size() == 1);
}

TEST_CASE("ext10_gtp: force waits for genmove's reply; reply attribution stays right") {
    Rig     r;
    LogSink search{r.log, "search"};
    REQUIRE(r.e.startSearch(emptyBoard(), Limits{}, search));
    CHECK(r.t.lines().back() == "4 genmove B");
    const auto before = r.t.lines();
    LogSink    cmd{r.log, "cmd"};
    CHECK(r.e.execute(setParam(), cmd).error() == EngineError::Busy);
    REQUIRE(r.e.execute(setParam(), cmd, {.force = true}));
    CHECK(r.t.lines() == before);  // stop() is a no-op and the command waits
    CHECK(r.e.execute(setParam(), cmd, {.force = true}).error() == EngineError::Busy);
    r.reply("=1");
    r.reply("=2");
    r.reply("=3");
    r.reply("=4 H8");
    CHECK(after(r.log.get(), "search best") ==
          Lines{"search best", "w kata-set-param maxVisits 100"});

    // The command's id-less reply is taken by its raw entry; an id-less `?` for the next
    // search's setup still ends that search (AUD-04).
    r.reply("=");
    LogSink next{r.log, "next"};
    REQUIRE(r.e.startSearch(emptyBoard(), Limits{}, next));
    r.reply("?");
    REQUIRE(next.get().size() == 1);
    CHECK(std::holds_alternative<ErrorEvent>(next.get()[0]));
    CHECK(cmd.get().empty());
}

TEST_CASE("ext10_gtp: close drops a queued command with one ErrorEvent") {
    Rig     r;
    LogSink search{r.log, "search"};
    REQUIRE(r.e.startSearch(emptyBoard(), Limits{}, search));
    LogSink cmd{r.log, "cmd"};
    REQUIRE(r.e.execute(setParam(), cmd, {.force = true}));
    r.t.closeNow();
    CHECK(after(r.log.get(), "search error") == Lines{"search error", "cmd error"});
}

TEST_CASE("ext10_gtp: a failed queued write reports with no lock, so the sink may call back") {
    Rig     r;
    LogSink search{r.log, "search"};
    REQUIRE(r.e.startSearch(emptyBoard(), Limits{}, search));
    LogSink           cmd{r.log, "cmd"};
    std::atomic<bool> sent{false};
    bool              sentInTime = false;
    std::thread       caller;
    cmd.hook = [&](const SearchEvent&) {
        // Another thread uses the engine while this callback runs; it must not wait for it.
        caller = std::thread([&] { sent = r.e.sendRaw("name").has_value(); });
        for (int i = 0; i < 200 && !sent; ++i) std::this_thread::sleep_for(5ms);
        sentInTime = sent;
    };
    REQUIRE(r.e.execute(setParam(), cmd, {.force = true}));
    r.t.failOn = "kata-set-param maxVisits 100";
    r.reply("=1");
    r.reply("=2");
    r.reply("=3");
    r.reply("=4 H8");
    REQUIRE(caller.joinable());
    caller.join();
    REQUIRE(cmd.get().size() == 1);
    CHECK(std::get<ErrorEvent>(cmd.get()[0]).message == "command could not be sent");
    CHECK(sentInTime);
}
