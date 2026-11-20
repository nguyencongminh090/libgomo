#pragma once

#include <vector>

#include "pygomo/core/engine_port.hpp"

namespace pygomo {

/// Everything one blocking search reported, split by kind.
struct SearchOutcome {
    std::vector<InfoEvent> info{};      ///< Progress reports, in arrival order.
    std::vector<SearchEvent> other{};   ///< Message and unparsed lines seen during the search.
    SearchEvent terminal{};             ///< BestMove, Swap, Pass, Resign or Error.
};

/// Runs one search and waits for its terminal event. Built on EnginePort::startSearch, so it
/// works with every adapter. Returns the start error if the search could not begin.
///
/// Blocks the calling thread until the terminal event arrives, so it never returns for an
/// engine that never answers; wrap that engine in DeadlineEngine first. Must not be called
/// from the thread that delivers the engine's events.
[[nodiscard]] std::expected<SearchOutcome, EngineError> search(
    EnginePort& engine, const Position& position, const Limits& limits);

}  // namespace pygomo
