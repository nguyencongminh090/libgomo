#pragma once

#include <compare>

#include "pygomo/core/side.hpp"

namespace pygomo {

/// A board point. 0-based, origin at the top-left, x grows right, y grows down.
/// Adapters convert to and from each protocol's own notation; the core never sees it.
struct Coord {
    int x{};
    int y{};

    constexpr auto operator<=>(const Coord&) const = default;
};

struct Move {
    Coord coord{};
    Side side{Side::Black};

    constexpr auto operator<=>(const Move&) const = default;
};

}  // namespace pygomo
