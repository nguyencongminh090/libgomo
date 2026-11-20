#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

namespace pygomo {

/// Search limits for one request. An empty field means "no limit from the caller";
/// the engine then applies its own default.
struct Limits {
    std::optional<std::chrono::milliseconds> turnTime{};   ///< Time for this move.
    std::optional<std::chrono::milliseconds> matchTime{};  ///< Time budget for the whole game.
    std::optional<int>                       maxDepth{};   ///< Search depth, in plies.
    std::optional<std::int64_t>              maxNodes{};   ///< Nodes to search.

    constexpr bool operator==(const Limits&) const = default;
};

}  // namespace pygomo
