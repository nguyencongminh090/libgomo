#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "pygomo/core/coord.hpp"

namespace pygomo {

/// A typed argument or field for engine extensions (command parameters, custom event fields,
/// rule extensions). The set is closed; a shape outside it is built as a line by the engine's
/// own encoder instead.
using Value = std::variant<bool, std::int64_t, double, std::string, Coord, Move, std::vector<Coord>,
                           std::vector<Move>>;

/// Default text form of `value`; an engine encoder may format differently.
/// bool: `1` / `0`. Integers and doubles: shortest decimal text. Coord: `x,y` (0-based, as in
/// the core). Move: `x,y,1` for Black and `x,y,2` for White. Vectors: elements joined by a
/// space. A string is returned as is.
[[nodiscard]] std::string format(const Value& value);

}  // namespace pygomo
