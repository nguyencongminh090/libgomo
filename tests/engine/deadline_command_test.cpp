// EXT-10: DeadlineEngine checks a command before keeping it during its own search.
#include <doctest.h>

#include <cstdint>
#include <string>

#include "../support/command_fakes.hpp"
#include "pygomo/engine/deadline_engine.hpp"
#include "pygomo/gtp/gtp_engine.hpp"

using namespace pygomo;
using namespace pygomo::testing;

TEST_CASE("ext10_deadline_check: force during a search refuses an unsendable command at once") {
    Log            log;
    FakeTransport  t{log};
    GtpEngine      gtp{t};
    DeadlineEngine e{gtp};
    REQUIRE(gtp.open());
    auto board = Position::create(15, 15);
    REQUIRE(board);
    LogSink search{log, "search"};
    REQUIRE(e.startSearch(*board, Limits{}, search));
    const auto before = t.lines();

    LogSink cmd{log, "cmd"};
    Command analyze{.name = "kata-genmove_analyze", .completion = Command::Search{}};
    Command noDone{.name = "x", .completion = Command::Until{}};
    for (const Command& c : {Command{}, noDone, analyze}) {
        CHECK_FALSE(e.canExecute(c));
        CHECK(e.execute(c, cmd, {.force = true}).error() == EngineError::Unsupported);
    }
    CHECK(t.lines() == before);
    CHECK(cmd.get().empty());

    Command param{.name = "kata-set-param", .args = {std::string{"maxVisits"}, std::int64_t{9}}};
    CHECK(e.canExecute(param));
    CHECK(e.execute(param, cmd, {.force = true}));  // kept until the search ends
}
