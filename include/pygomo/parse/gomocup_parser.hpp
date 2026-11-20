#pragma once

#include <string_view>

#include "pygomo/core/search_event.hpp"

namespace pygomo::parse {

/// Turns one line an engine printed into a neutral event. Pure and total: it never throws and
/// never drops text; free text after MESSAGE comes back as MessageEvent; a line it cannot
/// classify comes back as UnparsedEvent holding the line.
/// Coordinates are 0-based like Coord; the caller (the adapter) checks them against the board size.
[[nodiscard]] SearchEvent parseGomocupLine(std::string_view line);

}  // namespace pygomo::parse
