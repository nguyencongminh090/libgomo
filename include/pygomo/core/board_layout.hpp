#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "pygomo/core/coord.hpp"
#include "pygomo/core/value.hpp"

namespace pygomo {

/// The structure of a board beyond its size: wrapping edges, cells that take no stone and
/// linked cells. A structure, not a rule: moves are played the same way on any layout.
/// The core checks only that every cell is on the board; whether an engine can play a layout
/// (torus size, portals, Renju on a torus) is the engine's decision.
struct BoardLayout {
    bool               torus{false};  ///< Both edges wrap (Rapfi `INFO TORUS 1`).
    std::vector<Coord> blocked{};     ///< WALL cells: no stone can be played on them.
    std::vector<std::pair<Coord, Coord>> portals{};  ///< Linked cell pairs.
    /// Structure the core does not model, keyed by a name the engine's encoder knows.
    std::map<std::string, Value> extensions{};

    bool operator==(const BoardLayout&) const = default;
};

}  // namespace pygomo
