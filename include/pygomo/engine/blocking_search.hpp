#pragma once

#include <vector>

#include "pygomo/core/engine_port.hpp"

namespace pygomo {

struct SearchOutcome {
    std::vector<InfoEvent> info{};      ///< Progress reports, in arrival order.
    std::vector<SearchEvent> other{};   ///< Message and unparsed lines seen during the search.
    SearchEvent terminal{};             ///< BestMove, Swap, Pass or Error.
};

/// Runs one search and waits for its terminal event. Built on EnginePort::startSearch, so it
/// works with every adapter. Returns the start error if the search could not begin.
[[nodiscard]] std::expected<SearchOutcome, EngineError> search(
    EnginePort& engine, const Position& position, const Limits& limits);

}  // namespace pygomo
