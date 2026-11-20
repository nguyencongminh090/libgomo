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
    /// What `value` measures.
    enum class Kind {
        Eval, ///< A heuristic evaluation; higher is better for the side to move.
        MateIn, ///< The side to move wins in `value` moves.
        MatedIn ///< The side to move loses in `value` moves.
    };

    Kind kind{Kind::Eval}; ///< How to read `value`.
    int value{};  ///< Engine units for Eval; number of moves for MateIn / MatedIn.

    constexpr bool operator==(const Score&) const = default;
};

/// Progress report during a search. Every field is optional: engines differ in what they print.
struct InfoEvent {
    std::optional<int> depth{}; ///< Search depth reached, in plies.
    std::optional<std::int64_t> nodes{}; ///< Nodes searched so far.
    std::optional<Score> score{}; ///< Evaluation of the best line.
    std::optional<std::chrono::milliseconds> elapsed{}; ///< Time since the search started.
    std::vector<Coord> principalVariation{}; ///< Best line, first move first; empty if not sent.

    bool operator==(const InfoEvent&) const = default;
};

/// The engine's chosen move. Terminal.
struct BestMoveEvent {
    Coord coord{}; ///< Where the engine plays.

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

/// The search failed: the engine reported a fatal error, went away, or did not answer in time.
/// Terminal.
struct ErrorEvent {
    std::string message{}; ///< Human-readable reason; not meant to be parsed.

    bool operator==(const ErrorEvent&) const = default;
};

/// The engine resigned. Terminal; the game is lost for the engine's side, nothing more is implied.
struct ResignEvent {
    constexpr bool operator==(const ResignEvent&) const = default;
};

/// Free text the engine sent on purpose (e.g. Gomocup `MESSAGE <text>` that is not an info shape).
/// Not terminal; the text is kept raw.
struct MessageEvent {
    std::string text{}; ///< The text after a `MESSAGE ` prefix, else the whole line.

    bool operator==(const MessageEvent&) const = default;
};

/// A line the parser did not understand at all. Kept so nothing an engine says is silently lost.
struct UnparsedEvent {
    std::string line{}; ///< The whole line, without the line ending.

    bool operator==(const UnparsedEvent&) const = default;
};

/// Everything an EventSink can receive. BestMove, Swap, Pass, Resign and Error end a search.
using SearchEvent = std::variant<InfoEvent, BestMoveEvent, SwapEvent, PassEvent, ResignEvent,
                                 ErrorEvent, MessageEvent, UnparsedEvent>;

}  // namespace pygomo
