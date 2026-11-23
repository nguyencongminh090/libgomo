#include <doctest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "gomo/core/command_port.hpp"

using namespace gomo;

using Lines = std::vector<std::string>;

TEST_CASE("ext10_lines: name and typed args form one line; None is the default completion") {
    Command c{.name = "INFO", .args = {std::string{"VCTHREAD"}, std::int64_t{2}}};
    CHECK(commandLines(c) == Lines{"INFO VCTHREAD 2"});
    CHECK(std::holds_alternative<Command::None>(c.completion));
}

TEST_CASE("ext10_lines: a block of coordinates follows the head, then the terminator") {
    Command c{.name       = "YXANALZ",
              .block      = {Coord{7, 7}, Coord{8, 9}},
              .terminator = "DONE",
              .completion = Command::Search{}};
    CHECK(commandLines(c) == Lines{"YXANALZ", "7,7", "8,9", "DONE"});
}

TEST_CASE("ext10_lines: explicit lines are sent as is instead of name and args") {
    Command c{.name = "ignored", .lines = {"A 1", "B"}, .args = {true}, .terminator = "END"};
    CHECK(commandLines(c) == Lines{"A 1", "B", "END"});
}

TEST_CASE("ext10_lines: an empty command has no line") {
    CHECK(commandLines(Command{}).empty());
    Command argsOnly{.args = {std::int64_t{3}, false}};
    CHECK(commandLines(argsOnly) == Lines{"3 0"});
}
