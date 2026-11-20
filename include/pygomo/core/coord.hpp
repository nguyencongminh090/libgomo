#pragma once

#include <compare>

#include "pygomo/core/side.hpp"

namespace pygomo {

/// A board point. 0-based, origin at the top-left, x grows right, y grows down.
/// Adapters convert to and from each protocol's own notation; the core never sees it.
struct Coord {
    int x{};  ///< Column, 0 = leftmost.
    int y{};  ///< Row, 0 = topmost.

    constexpr auto operator<=>(const Coord&) const = default;
};

/// A stone placed by one side.
struct Move {
    Coord coord{};            ///< Where the stone was placed.
    Side  side{Side::Black};  ///< Who placed it.

    constexpr auto operator<=>(const Move&) const = default;
};

}  // namespace pygomo
