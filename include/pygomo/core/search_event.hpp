#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "pygomo/core/coord.hpp"

namespace pygomo {

/// An engine's evaluation of the position, from the side to move's point of view.
struct Score {
    enum class Kind { Eval, MateIn, MatedIn };

    Kind kind{Kind::Eval};
    int value{};  ///< Engine units for Eval; number of moves for MateIn / MatedIn.

    constexpr bool operator==(const Score&) const = default;
};

/// Progress report during a search. Every field is optional: engines differ in what they print.
struct InfoEvent {
    std::optional<int> depth{};
    std::optional<std::int64_t> nodes{};
    std::optional<Score> score{};
    std::optional<std::chrono::milliseconds> elapsed{};
    std::vector<Coord> principalVariation{};

    bool operator==(const InfoEvent&) const = default;
};

struct BestMoveEvent {
    Coord coord{};

    constexpr bool operator==(const BestMoveEvent&) const = default;
};

/// The engine chose to swap sides in an opening protocol.
struct SwapEvent {
    constexpr bool operator==(const SwapEvent&) const = default;
};

/// The engine passed. Gomoku has no pass; callers may treat this as an error.
struct PassEvent {
    constexpr bool operator==(const PassEvent&) const = default;
};

struct ErrorEvent {
    std::string message{};

    bool operator==(const ErrorEvent&) const = default;
};

/// A line the parser did not recognise. Kept so nothing an engine says is silently lost.
struct UnparsedEvent {
    std::string line{};

    bool operator==(const UnparsedEvent&) const = default;
};

using SearchEvent =
    std::variant<InfoEvent, BestMoveEvent, SwapEvent, PassEvent, ErrorEvent, UnparsedEvent>;

}  // namespace pygomo
