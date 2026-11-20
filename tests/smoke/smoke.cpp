// Smoke check: the core links and one short game behaves. Exit code 0 means pass.
#include <cstdio>

#include "pygomo/core/limits.hpp"
#include "pygomo/core/position.hpp"
#include "pygomo/core/rules.hpp"
#include "pygomo/core/search_event.hpp"

// Prints the failed step and returns false so run() can bail out in one line.
namespace {

bool fail(const char* what) {
    std::fprintf(stderr, "smoke failed: %s\n", what);
    return false;
}

bool run() {
    using namespace pygomo;

    auto position = Position::create(15, 15);
    if (!position) return fail("create 15x15");
    for (Coord c : {Coord{7, 7}, Coord{8, 7}, Coord{7, 8}}) {
        if (!position->play(c)) return fail("play legal move");
    }
    if (position->sideToMove() != Side::White) return fail("side to move after three moves");
    if (position->play({7, 7})) return fail("occupied point must be rejected");

    const Rules rules{BoardRule::Renju, OpeningRule::Swap2, 225};
    if (!validate(rules)) return fail("valid rules");

    const Limits limits{.maxDepth = 10};
    if (!limits.maxDepth) return fail("limits");

    const SearchEvent event = BestMoveEvent{{8, 8}};
    if (!std::holds_alternative<BestMoveEvent>(event)) return fail("event variant");
    return true;
}

}  // namespace

int main() { return run() ? 0 : 1; }
